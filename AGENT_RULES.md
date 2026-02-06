# Agent Behavior Rules

1. **Transparency**: Always show the files changed and provide the diff of the changes in the response.
2. **Verification**: Run a test compile (`python -m platformio run -e esp32dev`) after EVERY code change.
3. **Self-Correction**: If the test compile fails, immediately analyze the error and fix it before asking for further instructions.
