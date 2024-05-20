"""
Copyright (c) 2024, Qualcomm Innovation Center, Inc. All rights reserved.

SPDX-License-Identifier: BSD-3-Clause
"""

import sys
from tokenizers import Tokenizer
tokenizer = Tokenizer.from_pretrained("openai/clip-vit-base-patch32")
tokenizer.save("../data/openai-clip-vit-base-patch32")

index = 0
print("Enter prompt:", file=sys.stderr)
for line in sys.stdin:
    prompt = line.rstrip()
    if not prompt:
        break

    index += 1
    token_ids = tokenizer.encode(prompt).ids
    print(f"prompt_{index}=\"{prompt}\"")
    print(f"token_ids_{index}={token_ids}")
    print("Enter prompt:", file=sys.stderr)
