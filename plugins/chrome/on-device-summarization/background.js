chrome.sidePanel
  .setPanelBehavior({ openPanelOnActionClick: true })
  .catch((error) => console.error(error));

chrome.tabs.onActivated.addListener((activeInfo) => {
  showSummary(activeInfo.tabId);
});
chrome.tabs.onUpdated.addListener(async (tabId) => {
  showSummary(tabId);
});

var port = chrome.runtime.connectNative('com.quic.aisummarization');
port.onMessage.addListener(function (msg) {
  console.log('Received' + msg);
});
port.onDisconnect.addListener(function () {
  console.log('Disconnected');
});
port.postMessage({server: "localhost", port: "8002", model: "llama-v3.1-8b"});

async function showSummary(tabId) {
  const tab = await chrome.tabs.get(tabId);
  if (!tab.url.startsWith('http')) {
    return;
  }
  const injection = await chrome.scripting.executeScript({
    target: { tabId },
    files: ['scripts/extract-content.js']
  });
  chrome.storage.session.set({ pageContent: injection[0].result });
}
