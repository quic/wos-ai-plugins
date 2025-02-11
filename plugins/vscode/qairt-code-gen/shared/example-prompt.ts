import { ProfileName } from "./profile";

enum ExamplePrompt {
  EMAIL_VALIDATION = `#Write a program to validate email address and write tests for this method
  `,

  FIBONACCI_NUMBERS = `#Write a python function to generate the nth fibonacci number.
  `,
  COMMENTS_FOR_EMAIL_VALIDATION = `# Write short comments for the below method
def test_validate_email():

    valid_email = 'john.doe@example.com'
    result = validate_email(valid_email)
    assert result == True, 'Email address is valid'

    invalid_email = 'john.doeexample.com'
    result = validate_email(invalid_email)
    assert result == False, 'Email address is invalid'
`,

    DEBUG_RECURSION_CODE = `#Where is the bug in this code?
 
def fib(n):
    if n <= 0:
        return n
    else:
        return fib(n-1) + fib(n-2)
`,

    DEBUG_LAMBDA_CODE = `This function should return a list of lambda functions that compute successive powers of their input, but it doesn’t work:
 
def power_funcs(max_pow):
    return [lambda x:x**k for k in range(1, max_pow+1)]
 
the function should be such that [h(2) for f in powers(3)] should give [2, 4, 8], but it currently gives [8,8,8]. What is happening here?
`,

    TEST_CASE_UNIQUE_ELEMENTS = `Your task is to write 2 tests to check the correctness of a function that solves a programming problem.
The tests must be between [TESTS] and [/TESTS] tags.
You must write the comment "#Test case n:" on a separate line directly above each assert statement, where n represents the test case number, starting from 1 and increasing by one for each subsequent test case.
 
Problem: Write a Python function to get the unique elements of a list.
`,

    OPTIMIZE_SORTING = `Can you optimize this code? Explain what you have optimized and why?
arr = [5, 2, 8, 7, 1];     
temp = 0;    
     
#Sort the array in ascending order    
for i in range(0, len(arr)):    
    for j in range(i+1, len(arr)):    
        if(arr[i] > arr[j]):    
            temp = arr[i];    
            arr[i] = arr[j];    
            arr[j] = temp;
`,

REFACTOR_CODE = `#Can you refactor this code:
numbers = [-1, -2, -4, 0, 3, -7]
has_positives = False
for n in numbers:
    if n > 0:
        has_positives = True
        break
`
}

export enum ExamplePromptName {
  EMAIL_VALIDATION = 'Email validation',
  FIBONACCI_NUMBERS = 'Fibonacci numbers',
  COMMENTS_FOR_EMAIL_VALIDATION = 'Comments for email validation',
  DEBUG_RECURSION_CODE = 'Debug recursion code',
  DEBUG_LAMBDA_CODE = 'Debug lambda code',
  TEST_CASE_UNIQUE_ELEMENTS = "Unique elements test case",
  OPTIMIZE_SORTING = "Optimize sorting",
  REFACTOR_CODE = "Refactor code"
}

export const EXAMPLE_PROMPT_NAME_TO_PROMPT_MAP: Record<ExamplePromptName, ExamplePrompt> = {
  [ExamplePromptName.EMAIL_VALIDATION]: ExamplePrompt.EMAIL_VALIDATION,
  [ExamplePromptName.FIBONACCI_NUMBERS]: ExamplePrompt.FIBONACCI_NUMBERS,
  [ExamplePromptName.COMMENTS_FOR_EMAIL_VALIDATION]: ExamplePrompt.COMMENTS_FOR_EMAIL_VALIDATION,
  [ExamplePromptName.DEBUG_RECURSION_CODE]: ExamplePrompt.DEBUG_RECURSION_CODE,
  [ExamplePromptName.DEBUG_LAMBDA_CODE]: ExamplePrompt.DEBUG_LAMBDA_CODE,
  [ExamplePromptName.TEST_CASE_UNIQUE_ELEMENTS]: ExamplePrompt.TEST_CASE_UNIQUE_ELEMENTS,
  [ExamplePromptName.OPTIMIZE_SORTING]: ExamplePrompt.OPTIMIZE_SORTING,
  [ExamplePromptName.REFACTOR_CODE]: ExamplePrompt.REFACTOR_CODE,
};

export const EXAMPLE_PROMPT_NAME_TO_PROFILE: Record<ExamplePromptName, ProfileName> = {
  [ExamplePromptName.EMAIL_VALIDATION]: ProfileName.CODE_GENERATION,
  [ExamplePromptName.FIBONACCI_NUMBERS]: ProfileName.CODE_GENERATION,
  [ExamplePromptName.COMMENTS_FOR_EMAIL_VALIDATION]: ProfileName.CODE_GENERATION,
  [ExamplePromptName.DEBUG_RECURSION_CODE]: ProfileName.CODE_DEBUGGER,
  [ExamplePromptName.DEBUG_LAMBDA_CODE]: ProfileName.CODE_DEBUGGER,
  [ExamplePromptName.TEST_CASE_UNIQUE_ELEMENTS]: ProfileName.TEST_EXPERT,
  [ExamplePromptName.OPTIMIZE_SORTING]: ProfileName.CODE_REFACTOR,
  [ExamplePromptName.REFACTOR_CODE]: ProfileName.CODE_REFACTOR,
};
