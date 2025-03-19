import DOMPurify from 'dompurify';
import { marked } from 'marked';

// The underlying model has a context of 1,024 tokens, out of which 26 are used by the internal prompt,
// leaving about 998 tokens for the input text. Each token corresponds, roughly, to about 4 characters, so 4,000
// is used as a limit to warn the user the content might be too long to summarize.
const MAX_MODEL_CHARS = 15000;

let pageContent = '';

const summaryElement = document.body.querySelector('#summary');
const warningElement = document.body.querySelector('#warning');
const summaryTypeSelect = document.querySelector('#type');
const summaryFormatSelect = document.querySelector('#format');
const summaryLengthSelect = document.querySelector('#length');

function onConfigChange() {
  const oldContent = pageContent;
  pageContent = '';
  onContentChange(oldContent);
}

[summaryTypeSelect, summaryFormatSelect, summaryLengthSelect].forEach((e) =>
  e.addEventListener('change', onConfigChange)
);

chrome.storage.session.get('pageContent', ({ pageContent }) => {
  onContentChange(pageContent);
});

chrome.storage.session.onChanged.addListener((changes) => {
  const pageContent = changes['pageContent'];
  onContentChange(pageContent.newValue);
});

async function onContentChange(newContent) {
  if (pageContent == newContent) {
    // no new content, do nothing
    return;
  }
  pageContent = newContent;
  let summary;
  if (newContent) {
    if (newContent.length > MAX_MODEL_CHARS) {
      updateWarning(
        `Text is too long for summarization with ${newContent.length} characters
         (maximum supported content length is ${MAX_MODEL_CHARS} characters). Trimming the excess content.`
      );
      newContent = newContent.substring(0, MAX_MODEL_CHARS)
    } else {
      updateWarning('');
    }
    showSummary('Loading...');
    summary = await generateSummary(newContent);
  } else {
    summary = "There's nothing to summarize";
  }
  showSummary(summary);
}

async function generateSummary(text) {
  try {
    console.log("Content length: " + text.length);
    fetch('http://localhost:8002/api/health')
        .then(response => {
          if (!response.ok) {
            throw new Error('Network response was not ok ' + response.statusText);
          }
          console.log(response)
        })
        .catch(error => {
          console.error('There has been a problem with your fetch operation:', error);
        });
    const response = await fetch('http://localhost:8002/api/generate_stream', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json'
        },
        body: JSON.stringify({
          ask: text,
          system_prompt: 'You are an expert in text summarization. Summarize '+  summaryTypeSelect.value + ' in ' + summaryLengthSelect.value +
           ' the given text from webpage. Output in ' + summaryFormatSelect.value
        })
      });

    if (!response.ok) {
      throw new Error('Network response was not ok ' + response.statusText);
    }
    const reader = response.body.getReader();
    const decoder = new TextDecoder('utf-8');
    let result = '';

    function read() {
      return reader.read().then(({ done, value }) => {
        if (done) {
          console.log('Stream complete');
          return;
        }
        result += decoder.decode(value, { stream: true });
        // console.log(result);
        showSummary(result);
        return read();
      });
    }
    await read();
    return result;
  } catch (e) {
    console.log('Summary generation failed');
    console.error(e);
    return 'Error: ' + e.message;
  }
}


async function showSummary(text) {
  summaryElement.innerHTML = DOMPurify.sanitize(marked.parse(text));
}

async function updateWarning(warning) {
  warningElement.textContent = warning;
  if (warning) {
    warningElement.removeAttribute('hidden');
  } else {
    warningElement.setAttribute('hidden', '');
  }
}
