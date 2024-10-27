# ESPIDF Google Gemini LLM Prompting Component

This project is a REST API component for prompting the Google Gemini language model from an ESP32 microcontroller.

## Features

| Feature                          | Description                                         | Status  |
|----------------------------------|-----------------------------------------------------|---------|
| **Text-Only Prompting**          | Text only prompting(oneshot)   | ✅      |
| **Text with Attached File Prompting** |                                                   |         |
| - Image                          | Support for prompting with image files.            | ⚠️      |
| - Document                       | Support for prompting with document files.         | ⚠️      |
| - Media                          | Support for prompting with audio/video.            | ⚠️      |
| **Text with Uploaded File Prompting** | Prompts on files uploaded to gemini's storage server. | ⚠️      |
| **Structure Response Prompting** | Prompts with specified output response in json | ⚠️      |
| **Chat (text and files)** | Model remembers previous prompts  | ✅      |

- ✅ **Stable**: Feature is stable and fully functional.
- 🔄 **Working**: Feature is actively being developed and tested.
- ⚠️ **WIP**: Work in Progress, features are still being worked on.


## Using
1. Include as you would an ESPIDF component in your project. [See how.](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/build-system.html)

## Resources
1. [**Component and Documentation**]()
2. **Examples**
   - [simple_chat_client]()
   - [text_wt_file_prompting]()
   - [vision]()
3. **Gemini API documentation**

## Issues and Contributing
Open issues/send a PR.