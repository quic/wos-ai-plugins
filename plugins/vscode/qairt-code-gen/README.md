# QAIRT Code - VSCode extension for AI NPU Code Assistant with QAIRT

VSCode extension for helping developers writing code with AI code assistant.
QAIRT Code is working with Large Language Model for Chat (LLAMA-v3) deployed on local device.
The model runs completely on QAIRT NPU.

## Working with Extension

### Starting Server

On the extension side panel, choose your preferred model from the available options.

Current supported models:
1. Llama v2 7b
2. Llama v3.1 8b

Additionally, connection status shown on the VSCode Status Bar.
To check the connection manually, use the `Check Connection` button located on the side panel.

![vscode](https://github.com/quic/wos-ai-plugins/blob/main/plugins/vscode/qairt-code-gen/media/vscode.gif?raw=true)

## How to Install Extension

### Step 1: Get Llama 3.1 8b models
Generate the Llama 3.1 8b (and optionally Llama2 7b) models by following the instructions from AIHub models page:

https://github.com/quic/ai-hub-apps/tree/main/tutorials/llm_on_genie

https://github.com/quic/ai-hub-models/tree/main/qai_hub_models/models/llama_v3_1_8b_chat_quantized

Brief commands to be run:

Login to HuggingFace cli
```
huggingface-cli login
```
Login to AIHub account
```
qai-hub configure --api_token <token>
```

Install qai_hub_models
```
python3.10 -m venv llm_on_genie_venv
source llm_on_genie_venv/bin/activate
pip install -U "qai_hub_models[llama-v3-1-8b-chat-quantized]"
```

Export Llama 3.1 models (Expected time: ~2hrs)
```
mkdir -p genie_bundle

python -m qai_hub_models.models.llama_v3_1_8b_chat_quantized.export --device "Snapdragon X Elite CRD" --skip-inferencing --skip-profiling --output-dir genie_bundle
```

After completion of the context binary generation, you will find the below QNN model files.

```
genie_bundle
  |_  llama_v3_1_8b_chat_quantized_part_1_of_5.bin
  |_  llama_v3_1_8b_chat_quantized_part_2_of_5.bin
  |_  llama_v3_1_8b_chat_quantized_part_3_of_5.bin
  |_  llama_v3_1_8b_chat_quantized_part_4_of_5.bin
  |_  llama_v3_1_8b_chat_quantized_part_5_of_5.bin
```

### Step 2: Install Extension

Download ```qairt-code-completion-1.5.3.vsix``` from the [latest release](https://github.com/quic/wos-ai-plugins/releases/tag/v1.5.3-vscode).

Install the extension by opening VSCode and navigate to ```"Extensions" (in left-side menu)``` > ```Click on "..." (3 dots) at the top``` > ```Install from VSIX...``` > ```select qairt-code-completion-x.y.z.vsix file```.

### Step 3: Copy QNN model files

Copy the model files which were generated in step ```#1``` to the qairt-code-completion extension folder.
Copy all 5 Llama 3.1 8b files to C:\Users\USER\.vscode\extensions\quic.qairt-code-completion-1.5.3\out\server\models\llama-v3p1 folder as below.
```
llama-v3p1
  |_  llama_v3_1_8b_chat_quantized_part_1_of_5.bin
  |_  llama_v3_1_8b_chat_quantized_part_2_of_5.bin
  |_  llama_v3_1_8b_chat_quantized_part_3_of_5.bin
  |_  llama_v3_1_8b_chat_quantized_part_4_of_5.bin
  |_  llama_v3_1_8b_chat_quantized_part_5_of_5.bin
```

![model_folder](https://github.com/quic/wos-ai-plugins/blob/main/plugins/vscode/qairt-code-gen/media/model_folder.png?raw=true)

## Build Extension

### 1. Pre-requisites
Before you begin, ensure you have the following software installed on your system:

#### 1.1 Visual Studio 2022
Install Visual Studio 2022. Add the necessary Arm64 components while installing.

#### 1.2 Node js
Install Node.js from https://nodejs.org/en/download

### 2. Build the VS Extension

Create an QAIRT Code-Gen VSCode extension
```
./build.ps1 -qnn_sdk_root "C:\Qualcomm\AIStack\QAIRT\2.31.0.250130"
```

You will find qairt-code-completion-1.5.3.vsix extension file in this project directory, which can be installed in VSCode.


