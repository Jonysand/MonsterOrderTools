# coding:utf-8
import os

'''土办法
解析txt后print适合C#的字典初始化格式
然后复制粘贴到MonsterData的构造函数里
'''
def convert_monster_name_list():
    keys = set() # 老白还能写重复来？
    with open(os.path.join(os.path.dirname(__file__), r"点怪名单.txt"), "rb") as f:
        for line in f.readlines():
            line = line.strip()
            if not line:
                continue
            line = line.decode("utf-8")
            tokens = line.split("(")
            monster_name = tokens[0]
            monster_nicknames = []
            if len(tokens) > 1:
                monster_nicknames = tokens[1].strip(')').strip().split(" ")
            # 反向print
            if monster_nicknames:
                for nickname in monster_nicknames:
                    if nickname in keys:
                        continue
                    print("{@\"\\b" + nickname + "\\b\",\"" + monster_name + "\"},")
                    keys.add(nickname)
                if monster_name not in keys:
                    print("{@\"\\b" + monster_name + "\\b\",\"" + monster_name + "\"},")
                    keys.add(monster_name)
            else:
                if monster_name in keys:
                    continue
                print("{@\"\\b" + monster_name + "\\b\",\"" + monster_name + "\"},")
                keys.add(monster_name)


def get_monster_icons():
    import requests, re
    kiranico_monster_page_url = r"https://mhworld.kiranico.com/zh/monsters"
    ret = requests.get(kiranico_monster_page_url)
    start_search = False
    png_url_patern = r'src="(.*?_ID\.png)"'
    monster_name_tag_patern = r"<a[^>]*>(.*?)</a>"
    ret = ret.content.decode("utf-8")
    CUR_MONSTER_LINK = ""
    ret_dict = {}
    for line in ret.split('\n'):
        line = line.strip()
        if line.find("大型怪物") > 0:
            start_search = True
        if line.find("小型怪物") > 0 and start_search:
            break
        if not start_search:
            CUR_MONSTER_LINK = ""
            continue
        url_match = re.findall(png_url_patern, line)
        if url_match:
            CUR_MONSTER_LINK = url_match[0]
        else:
            if CUR_MONSTER_LINK:
                monster_name_match = re.findall(monster_name_tag_patern, line)
                if monster_name_match:
                    ret_dict[monster_name_match[0]] = CUR_MONSTER_LINK
            CUR_MONSTER_LINK = ""
    for monster_name, url in ret_dict.items():
        print("{@\"" + monster_name + "\",@\"" + url + "\"},")


def normalize_monstername(name):
    name = name.strip()
    if name.startswith("历战王"):
        name = name.replace("历战王", "")
    if name.startswith("历战"):
        name = name.replace("历战", "")
    if name.startswith("王"):
        name = name.replace("王", "")
    if name.startswith("普通"):
        name = name.replace("普通", "")
    if name.startswith("普"):
        name = name.replace("普", "")
    return name


def get_full_MHW_monster_json():
    import requests, re, json
    kiranico_monster_page_url = r"https://mhworld.kiranico.com/zh/monsters"
    ret = requests.get(kiranico_monster_page_url)
    start_search = False
    png_url_patern = r'src="(.*?_ID\.png)"'
    monster_name_tag_patern = r"<a[^>]*>(.*?)</a>"
    ret = ret.content.decode("utf-8")
    CUR_MONSTER_LINK = ""
    ret_dict = {}
    for line in ret.split('\n'):
        line = line.strip()
        if line.find("大型怪物") > 0:
            start_search = True
        if line.find("小型怪物") > 0 and start_search:
            break
        if not start_search:
            CUR_MONSTER_LINK = ""
            continue
        url_match = re.findall(png_url_patern, line)
        if url_match:
            CUR_MONSTER_LINK = url_match[0]
        else:
            if CUR_MONSTER_LINK:
                monster_name_match = re.findall(monster_name_tag_patern, line)
                if monster_name_match:
                    ret_dict.setdefault(monster_name_match[0], {})["icon_url"] = CUR_MONSTER_LINK
    all_nicknames = set()
    with open(os.path.join(os.path.dirname(__file__), r"点怪名单.txt"), "rb") as f:
        for line in f.readlines():
            line = line.strip()
            if not line:
                continue
            line = line.decode("utf-8")
            tokens = line.split("(")
            monster_name = normalize_monstername(tokens[0])
            monster_nicknames = []
            if len(tokens) > 1:
                monster_nicknames = tokens[1].strip(')').strip().split(" ")
            # 反向print
            print(monster_name)
            if monster_nicknames:
                for nickname in monster_nicknames:
                    nickname = normalize_monstername(nickname)
                    if nickname in all_nicknames:
                        continue
                    ret_dict.setdefault(monster_name, {}).setdefault("nick_names", []).append(nickname)
                    all_nicknames.add(nickname)
                if monster_name not in all_nicknames:
                    ret_dict.setdefault(monster_name, {}).setdefault("nick_names", []).append(monster_name)
                    all_nicknames.add(monster_name)
            else:
                if monster_name in all_nicknames:
                    continue
                ret_dict.setdefault(monster_name, {}).setdefault("nick_names", []).append(monster_name)
                all_nicknames.add(monster_name)

    with open(os.path.join(os.path.dirname(__file__), r"点怪名单.json"), "w", encoding="utf-8") as f:
        f.write(json.dumps(ret_dict, indent=2, ensure_ascii=False))

def reformat_json():
    import json
    with open(os.path.join(os.path.dirname(__file__), r"点怪名单.json"), "r", encoding="utf-8") as f:
        data = json.load(f)
    for monster_name in data:
        data[monster_name].setdefault("icon_url", "")
        data[monster_name].setdefault("默认历战等级", 0)
    with open(os.path.join(os.path.dirname(__file__), r"点怪名单.json"), "w", encoding="utf-8") as f:
        f.write(json.dumps(data, indent=2, ensure_ascii=False))

if __name__ == "__main__":
    reformat_json()
