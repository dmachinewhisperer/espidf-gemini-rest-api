# Chat client on the ESP32
LLM-backed chat client for the ESP32. Based on the 

## Setup
1. Must have gemini-rest-api component. Set the directory it is in, in the top-level CMakeLists.txt
2. Setup Wifi in ```menuconfig->Example Connection Configuration```
3. To improve user experience, set logging verbosity in ```Menuconfig->Component config-> Log output->Default log verbosity```. Set to ```No output``` or ```Error```

### Sample outputs
### Oneshot prompting (Model not keep session history)

#### **Prompt: Which year was elen musk born?**
Elon Musk was born in **1971**.
### Chatting (Model keeps session history)
#### **Prompt: write a short code to calculate factorial in python**
```python
def factorial(n):
  """Calculates the factorial of a non-negative integer.

  Args:
    n: The non-negative integer to calculate the factorial of.

  Returns:
    The factorial of n.
  """
  if n == 0:
    return 1
  else:
    return n * factorial(n-1)

# Get user input for the number
num = int(input("Enter a non-negative integer: "))

# Calculate and print the factorial
print(f"The factorial of {num} is: {factorial(num)}")
```

#### **Prompt: change the name of the function to my_factorial_testing**
```python
def my_factorial_testing(n):
  """Calculates the factorial of a non-negative integer.

  Args:
    n: The non-negative integer to calculate the factorial of.

  Returns:
    The factorial of n.
  """
  if n == 0:
    return 1
  else:
    return n * my_factorial_testing(n-1)

# Get user input for the number
num = int(input("Enter a non-negative integer: "))

# Calculate and print the factorial
print(f"The factorial of {num} is: {my_factorial_testing(num)}")
```

I've simply changed the function name from `factorial` to `my_factorial_testing` in both the definition and the call within the code.  The rest of the logic remains the same.