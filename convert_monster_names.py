# coding:utf-8
import os

'''土办法
解析txt后print适合C#的字典初始化格式
然后复制粘贴到MonsterData的构造函数里
'''
if __name__ == "__main__":
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
            else:
                if monster_name in keys:
                    continue
                print("{@\"\\b" + monster_name + "\\b\",\"" + monster_name + "\"},")
                keys.add(monster_name)