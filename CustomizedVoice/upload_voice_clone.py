#!/usr/bin/env python3
import requests
import json
import os
import sys

API_BASE = "https://api.minimaxi.com"
API_KEY = ""


def load_api_key():
    global API_KEY
    cred_path = os.path.join(os.path.dirname(__file__), "..", "scripts", "credentials.json")
    try:
        with open(cred_path, "r", encoding="utf-8") as f:
            creds = json.load(f)
            API_KEY = creds.get("minimax_tts_api_key", "")
    except Exception:
        pass
    if not API_KEY:
        API_KEY = os.environ.get("MINIMAX_API_KEY", "")


def upload_file(file_path):
    url = f"{API_BASE}/v1/files/upload"
    headers = {"Authorization": f"Bearer {API_KEY}"}

    if not os.path.exists(file_path):
        print(f"[ERROR] File not found: {file_path}")
        return None

    file_size = os.path.getsize(file_path)
    if file_size > 20 * 1024 * 1024:
        print(f"[ERROR] File size exceeds 20MB limit: {file_size} bytes")
        return None

    ext = os.path.splitext(file_path)[1].lower()
    if ext not in [".mp3", ".m4a", ".wav"]:
        print(f"[ERROR] Unsupported file format: {ext}")
        return None

    with open(file_path, "rb") as f:
        files = {"file": (os.path.basename(file_path), f, f"audio/{ext[1:]}")}
        data = {"purpose": "voice_clone"}
        resp = requests.post(url, headers=headers, files=files, data=data, timeout=120)

    if resp.status_code == 200:
        result = resp.json()
        file_id = result.get("file", {}).get("file_id")
        print(f"[PASS] File uploaded successfully, file_id: {file_id}")
        return file_id
    else:
        print(f"[ERROR] Upload failed: {resp.status_code} - {resp.text}")
        return None


def voice_clone(file_id, voice_id, text=None, model="speech-2.8-hd",
                need_noise_reduction=False, need_volume_normalization=False,
                aigc_watermark=False, language_boost=None, clone_prompt=None):
    url = f"{API_BASE}/v1/voice_clone"
    headers = {
        "Authorization": f"Bearer {API_KEY}",
        "Content-Type": "application/json"
    }

    payload = {
        "file_id": file_id,
        "voice_id": voice_id,
        "need_noise_reduction": need_noise_reduction,
        "need_volume_normalization": need_volume_normalization,
        "aigc_watermark": aigc_watermark
    }

    if text and model:
        payload["text"] = text
        payload["model"] = model

    if language_boost:
        payload["language_boost"] = language_boost

    if clone_prompt:
        payload["clone_prompt"] = clone_prompt

    resp = requests.post(url, headers=headers, json=payload, timeout=60)

    if resp.status_code == 200:
        result = resp.json()
        base_resp = result.get("base_resp", {})
        status_code = base_resp.get("status_code", -1)
        if status_code == 0:
            print(f"[PASS] Voice clone initiated successfully")
            demo_audio = result.get("demo_audio", "")
            if demo_audio:
                print(f"      Demo audio: {demo_audio}")
            return result
        else:
            print(f"[ERROR] Voice clone failed: {result}")
            return None
    else:
        print(f"[ERROR] Voice clone request failed: {resp.status_code} - {resp.text}")
        return None


def get_voice(voice_type="all"):
    url = f"{API_BASE}/v1/get_voice"
    headers = {
        "Authorization": f"Bearer {API_KEY}",
        "Content-Type": "application/json"
    }

    payload = {"voice_type": voice_type}
    resp = requests.post(url, headers=headers, json=payload, timeout=30)

    if resp.status_code == 200:
        result = resp.json()
        base_resp = result.get("base_resp", {})
        status_code = base_resp.get("status_code", -1)
        if status_code == 0:
            voices = {
                "system": result.get("system_voice", []),
                "voice_cloning": result.get("voice_cloning", []),
                "voice_generation": result.get("voice_generation", [])
            }
            return voices
        else:
            print(f"[ERROR] Get voice failed: {base_resp.get('status_msg', 'Unknown error')}")
            return None
    else:
        print(f"[ERROR] Get voice request failed: {resp.status_code} - {resp.text}")
        return None


def main():
    load_api_key()
    if not API_KEY:
        print("[ERROR] API key not found. Set MINIMAX_API_KEY environment variable or update credentials.json")
        sys.exit(1)

    if len(sys.argv) < 2:
        print("Usage:")
        print("  List voices: python upload_voice_clone.py list [voice_type]")
        print("    voice_type: system, voice_cloning, voice_generation, all (default: all)")
        print("  Upload only: python upload_voice_clone.py upload <audio_file>")
        print("  Clone voice: python upload_voice_clone.py clone <audio_file> <voice_id> [text]")
        sys.exit(1)

    command = sys.argv[1].lower()

    if command == "list":
        voice_type = sys.argv[2] if len(sys.argv) > 2 else "all"
        if voice_type not in ("system", "voice_cloning", "voice_generation", "all"):
            print(f"[ERROR] Invalid voice_type: {voice_type}")
            sys.exit(1)
        result = get_voice(voice_type)
        if result:
            for vtype, voices in result.items():
                if voices:
                    print(f"\n=== {vtype} ===")
                    for v in voices:
                        print(f"  voice_id: {v.get('voice_id')}")
                        if v.get("voice_name"):
                            print(f"    voice_name: {v.get('voice_name')}")
                        if v.get("description"):
                            print(f"    description: {', '.join(v.get('description', []))}")
                        if v.get("created_time"):
                            print(f"    created_time: {v.get('created_time')}")
        sys.exit(0 if result else 1)

    elif command == "upload":
        file_path = sys.argv[2]
        file_id = upload_file(file_path)
        if file_id:
            print(f"\nfile_id for voice_clone: {file_id}")
        sys.exit(0 if file_id else 1)

    elif command == "clone":
        if len(sys.argv) < 4:
            print("[ERROR] clone requires <audio_file> and <voice_id>")
            sys.exit(1)

        file_path = sys.argv[2]
        voice_id = sys.argv[3]
        text = sys.argv[4] if len(sys.argv) > 4 else None

        print(f"Uploading audio file: {file_path}")
        file_id = upload_file(file_path)
        if not file_id:
            sys.exit(1)

        print(f"Cloning voice with voice_id: {voice_id}")
        result = voice_clone(file_id, voice_id, text=text)
        sys.exit(0 if result else 1)

    else:
        print(f"[ERROR] Unknown command: {command}")
        sys.exit(1)


if __name__ == "__main__":
    main()
