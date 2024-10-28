# ESPIDF Google Gemini LLM Prompting Component

This project is a REST API component for prompting the Google's Gemini LLM from any ESP32-based microcontroller.

## Features

| Feature                          | Description                                         | Status  |
|----------------------------------|-----------------------------------------------------|---------|
| **Text-Only Prompting**          | Text only prompting(oneshot)   |✅      |
| **Text with Attached File Prompting** | Prompting with attached file encoded in base64      | ⚠️      |
| **Text with Uploaded File Prompting** | Prompts on files uploaded to gemini's storage server. | ✅      |
| **Structured Response Prompting** | Prompts with specified output response in json | ⚠️      |
| **Chat (text and files)** | Model remembers previous prompts  | ✅      |

- ✅ **Stable**: Feature is stable and fully functional.
- 🔄 **Working**: Feature is actively being developed and tested.
- ⚠️ **WIP**: Work in Progress, features are still being worked on.


## Using
Include as you would an ESPIDF component in your project. [See how.](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/build-system.html)

## Resources
1. [**Component**](https://github.com/dmachinewhisperer/espidf-gemini-rest-api/tree/master/gemini-rest-api) and [**Documentation**](https://github.com/dmachinewhisperer/espidf-gemini-rest-api/tree/master/gemini-rest-api#documentation-wip)
2. **Examples using the component**
   - [simple_chat_client](https://github.com/dmachinewhisperer/espidf-gemini-rest-api/tree/master/examples/simple-chat-client)
   - [prompting_wt_files](https://github.com/dmachinewhisperer/espidf-gemini-rest-api/tree/master/examples/simple-chat-client)
   - [vision]()
3. [**Gemini API documentation**](https://ai.google.dev/gemini-api/doc)

## Issues and Contributing
Open issues/send a PR.