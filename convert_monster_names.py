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

if __name__ == "__main__":
    get_monster_icons()