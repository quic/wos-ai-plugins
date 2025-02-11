
enum ProfileSystemPrompt {
  CODE_GENERATION = "Act as an expert python coder complete the python code to run properly using proper imports. Apart from comments, no additional explaination is needed. Provide code in code blocks with ```",
  CODE_REFACTOR = "You are an expert programmer that helps to refactor Python code.",
  TEST_EXPERT = "You are an expert programmer that helps write unit tests. Don't explain anything just write the tests.",
  CODE_DEBUGGER = "You are an expert programmer that helps to review Python code for bugs."
}

export enum ProfileName {
  CODE_GENERATION = 'Code Generation',
  CODE_REFACTOR = 'Code Refactor',
  TEST_EXPERT = 'Test Expert',
  CODE_DEBUGGER = 'Code Debugger',
}

export const PROFILE_NAME_TO_PROMPT_MAP: Record<ProfileName, ProfileSystemPrompt> = {
  [ProfileName.CODE_GENERATION]: ProfileSystemPrompt.CODE_GENERATION,
  [ProfileName.CODE_REFACTOR]: ProfileSystemPrompt.CODE_REFACTOR,
  [ProfileName.TEST_EXPERT]: ProfileSystemPrompt.TEST_EXPERT,
  [ProfileName.CODE_DEBUGGER]: ProfileSystemPrompt.CODE_DEBUGGER,
};
