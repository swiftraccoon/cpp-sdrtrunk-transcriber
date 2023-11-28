import re
import requests
import argparse
import os
import json


url = "http://127.0.0.1:5000/v1/chat/completions"

# Average length of a token, this is an estimation and might need adjustments
AVERAGE_TOKEN_LENGTH = 4
MAX_TOKEN_PER_SECTION = 4045

headers = {
    "Content-Type": "application/json"
}

history = []


def count_tokens(text):
    """
    Estimate the number of tokens in the text.
    """
    return len(text) // AVERAGE_TOKEN_LENGTH


def split_text_into_sections(text, max_tokens):
    sections = []
    current_section = ""
    current_token_count = 0

    for line in text.split('\n'):
        tokens = count_tokens(line)

        if current_token_count + tokens <= max_tokens:
            current_section += line + '\n'
            current_token_count += tokens
        else:
            sections.append(current_section.strip())
            current_section = line + '\n'
            current_token_count = tokens

    if current_section:
        sections.append(current_section.strip())

    return sections


def summarize_text(text, prompt=None):
    if prompt is None:
        prompt = """
        Supplied are transcriptions of radio traffic.
        You are writing a daily summary of high profile incidents for the Sheriff.
        List notable incidents that occurred in the transcriptions. 
        """
    history.append({"role": "user", "content": prompt + text})
    data = {
        "mode": "chat",
        "character": "911 Operator",
        "messages": history
    }

    try:
        response = requests.post(url, headers=headers, json=data, verify=False)
        response.raise_for_status()  # Check for HTTP errors
        response_json = response.json()
        assistant_message = response_json['choices'][0]['message']['content']
        history.append({"role": "assistant", "content": assistant_message})
        return assistant_message
    except requests.exceptions.HTTPError as http_err:
        print(f"HTTP error occurred: {http_err}")
    except json.decoder.JSONDecodeError as json_err:
        print(f"JSON decoding error occurred: {json_err}")
    except Exception as e:
        print(f"An unexpected error occurred: {e}")


def main():
    parser = argparse.ArgumentParser(description="Summarize text files")
    parser.add_argument("-d", "--date", required=True,
                        help="Date in YYYYMMDD format")
    parser.add_argument("-f", "--folder", required=True,
                        help="Folder path containing text files")
    args = parser.parse_args()

    date = args.date
    folder_path = args.folder
    print(f"Date: {date}")
    print(f"Folder path: {folder_path}")

    if not os.path.exists(folder_path):
        print("Folder path does not exist.")
        return

    txt_files = []
    for root, dirs, files in os.walk(folder_path):
        for file in files:
            if file.endswith(".txt") and re.match(rf"{date}", file):
                txt_files.append(os.path.join(root, file))

    if not txt_files:
        print(f"No .txt files found in the folder for the date {date}.")
        return

    all_text = ""
    for txt_file in txt_files:
        with open(os.path.join(folder_path, txt_file), 'r') as file:
            file_content = file.read()
            all_text += f"Filename: {txt_file}\n{file_content}\n\n"

    total_tokens = count_tokens(all_text)
    print(f"Total tokens in the text: {total_tokens}")

    sections = split_text_into_sections(all_text, MAX_TOKEN_PER_SECTION)

    summaries = []
    for i, section in enumerate(sections):
        prompt = """
        Supplied are transcriptions of radio traffic.
        You are writing a daily summary of high profile incidents for the Sheriff.
        List notable incidents that occurred in the transcriptions. 
        """
        summary = summarize_text(section, prompt=prompt)
        summaries.append(summary)

    for i, summary in enumerate(summaries):
        print(f"Summary for section {i + 1}:\n{summary}")


if __name__ == "__main__":
    main()
