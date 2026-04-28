#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
怪物图标URL转换脚本
将monster_list.json中的网络URL转换为zip内路径
"""

import json
import re
import zipfile
import shutil
from pathlib import Path
from collections import defaultdict

# 路径配置
CONFIG_DIR = Path(r'D:\VisualStudioProjects\JonysandMHDanmuTools\MonsterOrderWilds_configs')
JSON_PATH = CONFIG_DIR / 'monster_list.json'
ZIP_PATH = CONFIG_DIR / 'monster_icons.zip'
BACKUP_PATH = CONFIG_DIR / 'monster_list.json.bak'

# 读取zip文件列表
with zipfile.ZipFile(ZIP_PATH, 'r') as z:
    zip_files = [f.filename for f in z.infolist() if f.filename.endswith('.png')]

# 建立zip文件名映射（小写，用于匹配）
zip_map = {}
for zf in zip_files:
    fname = zf.split('/')[-1]
    # 提取基础英文名（移除前缀和后缀）
    base = fname
    base = re.sub(r'^(MHRS|MHRise|MHWI|MHWorld|MHWilds)_?', '', base)
    base = re.sub(r'_(Icon|Tempered|ArchTempered|Afflicted|Risen)\.png$', '', base)
    zip_map[base.lower()] = zf

print(f'Loaded {len(zip_files)} icons from zip')
print(f'Unique names: {len(zip_map)}')

# ==========================================
# em编号到zip路径的映射表
# 基于MHW怪物ID和实际怪物名称建立
# ==========================================
EM_ID_MAP = {
    # 飞龙种
    "em001": "MHRise/MHRS-Pink_Rathian_Icon.png",      # 樱火龙
    "em002": "MHRise/MHRS-Silver_Rathalos_Icon.png",    # 银火龙
    "em007": "MHRise/MHRS-Black_Diablos_Icon.png",      # 黑角龙
    "em011": "MHWilds/MHWilds-Kirin_Icon.png",          # 麒麟
    "em013": "MHWorld/MHWorld-Fatalis_Icon.png",        # 黑龙
    "em018": "MHRise/MHRS-Yian_Garuga_Icon.png",        # 黑狼鸟
    "em023": "MHRise/MHRS-Rajang_Icon.png",             # 金狮子
    "em024": "MHRise/MHRS-Kushala_Daora_Icon.png",      # 钢龙
    "em026": "MHRise/MHRS-Lunastra_Icon.png",           # 炎妃龙
    "em027": "MHRise/MHRS-Teostra_Icon.png",            # 炎王龙
    "em032": "MHRise/MHRS-Brute_Tigrex_Icon.png",       # 黑轰龙（使用轰龙替代）
    "em036": "MHWilds/MHWilds-Lavasioth_Icon.png",      # 熔岩龙
    "em037": "MHRise/MHRS-Nargacuga_Icon.png",          # 迅龙
    "em042": "MHRise/MHRS-Barioth_Icon.png",            # 冰牙龙
    "em043": "MHRise/MHRS-Deviljho_Icon.png",           # 恐暴龙
    "em044": "MHWilds/MHWilds-Barroth_Icon.png",        # 土砂龙
    "em045": "MHWilds/MHWilds-Uragaan_Icon.png",        # 爆锤龙
    "em050": "MHWorld/MHWorld-Alatreon_Icon.png",       # 煌黑龙
    "em057": "MHRise/MHRS-Zinogre_Icon.png",            # 雷狼龙
    "em063": "MHWilds/MHWilds-Brachydios_Icon.png",     # 碎龙
    "em080": "MHRise/MHRS-Glavenus_Icon.png",           # 斩龙
    "em100": "MHWilds/MHWilds-Anjanath_Icon.png",       # 蛮颚龙
    "em101": "MHWilds/MHWilds-Great_Jagras_Icon.png",   # 大贼龙
    "em102": "MHWilds/MHWilds-Pukei-Pukei_Icon.png",    # 毒妖鸟
    "em103": "MHWilds/MHWilds-Nergigante_Icon.png",     # 灭尽龙
    "em104": "MHWilds/MHWilds-Safi'jiiva_Icon.png",    # 冥赤龙
    "em105": "MHWilds/MHWilds-Xeno'jiiva_Icon.png",    # 冥灯龙
    "em106": "MHWilds/MHWilds-Zorah_Magdaros_Icon.png", # 熔山龙
    "em107": "MHWilds/MHWilds-Kulu-Ya-Ku_Icon.png",     # 搔鸟
    "em108": "MHWilds/MHWilds-Jyuratodus_Icon.png",     # 泥鱼龙
    "em109": "MHWilds/MHWilds-Tobi-Kadachi_Icon.png",   # 飞雷龙
    "em110": "MHWilds/MHWilds-Paolumu_Icon.png",        # 浮空龙
    "em111": "MHWilds/MHWilds-Legiana_Icon.png",        # 风漂龙
    "em112": "MHWilds/MHWilds-Girros_Icon.png",         # 大痹贼龙
    "em113": "MHWilds/MHWilds-Odogaron_Icon.png",       # 惨爪龙
    "em114": "MHWilds/MHWilds-Radobaan_Icon.png",       # 骨锤龙
    "em115": "MHWilds/MHWilds-Vaal_Hazak_Icon.png",     # 尸套龙
    "em116": "MHWilds/MHWilds-Dodogama_Icon.png",       # 岩贼龙
    "em117": "MHWilds/MHWilds-Kulve_Taroth_Icon.png",   # 绚辉龙
    "em118": "MHWilds/MHWilds-Bazelgeuse_Icon.png",     # 爆鳞龙
    "em120": "MHWilds/MHWilds-Tzitzi-Ya-Ku_Icon.png",   # 眩鸟
    "em121": "MHWilds/MHWilds-Behemoth_Icon.png",       # 贝希摩斯
    "em122": "MHWilds/MHWilds-Beotodus_Icon.png",       # 冰鱼龙
    "em123": "MHWilds/MHWilds-Banbaro_Icon.png",        # 猛牛龙
    "em124": "MHWilds/MHWilds-Velkhana_Icon.png",       # 冰呪龙
    "em125": "MHWilds/MHWilds-Namielle_Icon.png",       # 溟波龙
    "em126": "MHWilds/MHWilds-Shara_Ishvalda_Icon.png", # 天地煌啼龙
    "em127": "MHWilds/MHWilds-Leshen_Icon.png",         # 鹿首精
}

# 怪物中文名到zip路径的直接映射（用于无URL怪物和特殊情况）
# 格式: 中文名 -> zip内路径
MONSTER_NAME_MAP = {
    # MHRise怪物
    "霸主雌火龙": "MHRise/MHRS-Apex_Rathian_Icon.png",
    "霸主青熊兽": "MHRise/MHRS-Apex_Arzuros_Icon.png",
    "霸主角龙": "MHRise/MHRS-Apex_Diablos_Icon.png",
    "霸主火龙": "MHRise/MHRS-Apex_Rathalos_Icon.png",
    "霸主雷狼龙": "MHRise/MHRS-Apex_Zinogre_Icon.png",
    "霸主泡狐龙": "MHRise/MHRS-Apex_Mizutsune_Icon.png",
    
    # MHRise Sunbreak
    "爵银龙": "MHRS/MHRS-Malzeno_Icon.png",
    "原初形态爵银龙": "MHRS/MHRS-Primordial_Malzeno_Icon.png",
    "冰狼龙": "MHRS/MHRS-Lunagaron_Icon.png",
    "刚缠兽": "MHRS/MHRS-Garangolm_Icon.png",
    "电龙": "MHRS/MHRS-Astalos_Icon.png",
    "千刃龙": "MHRS/MHRS-Seregios_Icon.png",
    "将军镰蟹": "MHRS/MHRS-Shogun_Ceanataur_Icon.png",
    "大名盾蟹": "MHRS/MHRS-Daimyo_Hermitaur_Icon.png",
    "炽妃蜘蛛": "MHRS/MHRS-Pyre_Rakna-Kadaki_Icon.png",
    "绯天狗兽": "MHRS/MHRS-Blood_Orange_Bishaten_Icon.png",
    "熔翁龙": "MHRS/MHRS-Magma_Almudron_Icon.png",
    "焰狐龙": "MHRS/MHRS-Soulseer_Mizutsune_Icon.png",
    "月迅龙": "MHRS/MHRS-Lucent_Nargacuga_Icon.png",
    "红莲爆鳞龙": "MHRS/MHRS-Seething_Bazelgeuse_Icon.png",
    "嗟怨震天怨虎龙": "MHRS/MHRS-Scorned_Magnamalo_Icon.png",
    
    # 怪异化
    "怪异克服钢龙": "MHRS_Risen/MHRS-Risen_Kushala_Daora_Icon.png",
    "怪异克服霞龙": "MHRS_Risen/MHRS-Risen_Chameleos_Icon.png",
    "怪异克服炎王龙": "MHRS_Risen/MHRS-Risen_Teostra_Icon.png",
    "怪异克服天彗龙": "MHRS_Risen/MHRS-Risen_Crimson_Glow_Valstrax_Icon.png",
    "怪异克服天廻龙": "MHRS_Risen/MHRS-Risen_Shagaru_Magala_Icon.png",
    
    # MHWilds
    "刺花蜘蛛": "MHWilds/MHWilds-Lala_Barina_Icon.png",
    "辟兽": "MHWilds/MHWilds-Doshaguma_Icon.png",
    "护辟兽": "MHWilds/MHWilds-Guardian_Doshaguma_Icon.png",
    "风铗龙": "MHWilds/MHWilds-Hirabami_Icon.png",
    "沙海龙": "MHWilds/MHWilds-Balahara_Icon.png",
    "暗器蛸": "MHWilds/MHWilds-Xu_Wu_Icon.png",
    "白炽龙": "MHWilds/MHWilds-Zoh_Shia_Icon.png",
    "炎尾龙": "MHWilds/MHWilds-Quematrice_Icon.png",
    "护火龙": "MHWilds/MHWilds-Guardian_Rathalos_Icon.png",
    "桃毛兽王": "MHWilds/MHWilds-Congalala_Icon.png",
    "雪狮子王": "MHWilds/MHWilds-Blangonga_Icon.png",
    "护雷颚龙": "MHWilds/MHWilds-Guardian_Ebony_Odogaron_Icon.png",
    "锁刃龙": "MHWilds/MHWilds-Arkveld_Icon.png",
    "护凶爪龙": "MHWilds/MHWilds-Guardian_Fulgur_Anjanath_Icon.png",
    "铠龙": "MHWilds/MHWilds-Gravios_Icon.png",
    "火龙": "MHWilds/MHWilds-Rathalos_Icon.png",
    "黑蚀龙": "MHWilds/MHWilds-Gore_Magala_Icon.png",
    "狱焰蛸": "MHWilds/MHWilds-G.·I··O·r·g·a·d·o·r·m_Icon.png",
    "煌雷龙": "MHWilds/MHWilds-Rey_Dau_Icon.png",
    "雌火龙": "MHWilds/MHWilds-Rathian_Icon.png",
    "毒怪鸟": "MHWilds/MHWilds-Gypceros_Icon.png",
    "护锁刃龙": "MHWilds/MHWilds-Guardian_Arkveld_Icon.png",
    "泡狐龙": "MHWilds/MHWilds-Mizutsune_Icon.png",
    "波衣龙": "MHWilds/MHWilds-Uth_Duna_Icon.png",
    "缠蛙": "MHWilds/MHWilds-Chatacabra_Icon.png",
    "赫猿兽": "MHWilds/MHWilds-Ajarakan_Icon.png",
    "沼喷龙": "MHWilds/MHWilds-Rompopolo_Icon.png",
    "怪鸟": "MHWilds/MHWilds-Yian_Kut-Ku_Icon.png",
    "护雷颚龙": "MHWilds/MHWilds-Guardian_Fulgur_Anjanath_Icon.png",
    "护凶爪龙": "MHWilds/MHWilds-Guardian_Ebony_Odogaron_Icon.png",
    
    # 冰人鱼龙 - 需要确认
    "冰人鱼龙": "MHWilds/MHWilds-Aurora_Somnacanth_Icon.png",
    
    # 其他
    "岩龙": "MHWilds/MHWilds-Basarios_Icon.png",
    "霞龙": "MHRise/MHRS-Chameleos_Icon.png",
    "泥翁龙": "MHRise/MHRS-Almudron_Icon.png",
    "妃蜘蛛": "MHRise/MHRS-Rakna-Kadaki_Icon.png",
    "人鱼龙": "MHRise/MHRS-Somnacanth_Icon.png",
    "棘龙": "MHRise/MHRS-Espinas_Icon.png",
    "棘茶龙": "MHRise/MHRS-Flaming_Espinas_Icon.png",
    "天狗兽": "MHRise/MHRS-Bishaten_Icon.png",
    "赤甲兽": "MHRise/MHRS-Volvidon_Icon.png",
    "镰鼬龙王": "MHRise/MHRS-Great_Izuchi_Icon.png",
    "岚龙": "MHRS/MHRS-Amatsu_Icon.png",
    "水兽": "MHRise/MHRS-Royal_Ludroth_Icon.png",
    "奇怪龙": "MHRise/MHRS-Khezu_Icon.png",
    "雪鬼兽": "MHRise/MHRS-Goss_Harag_Icon.png",
    "眠狗龙王": "MHRise/MHRS-Great_Baggi_Icon.png",
    "混沌黑蚀龙": "MHRS/MHRS-Chaotic_Gore_Magala_Icon.png",
    "雷神龙": "MHRise/MHRS-Thunder_Serpent_Narwa_Icon.png",
    "百龙渊源雷神龙": "MHRise/MHRS-Narwa_the_Allmother_Icon.png",
    "伞鸟": "MHRise/MHRS-Aknosom_Icon.png",
    "天廻龙": "MHRS/MHRS-Shagaru_Magala_Icon.png",
    "风神龙": "MHRise/MHRS-Wind_Serpent_Ibushi_Icon.png",
    "白兔兽": "MHRise/MHRS-Lagombi_Icon.png",
    "毒狗龙王": "MHRise/MHRS-Great_Wroggi_Icon.png",
    "冥渊龙": "MHRS/MHRS-Gaismagorm_Icon.png",
    "怨虎龙": "MHRise/MHRS-Magnamalo_Icon.png",
    
    # 特殊怪物
    "极贝": "MHWilds/MHWilds-Behemoth_Icon.png",
    "贝希摩斯": "MHWilds/MHWilds-Behemoth_Icon.png",
    
    # 暂无图标的（保持空字符串）
    "烈祸月迅龙": "",
    "烈祸棘茶龙": "",
    "零式游星欧米茄": "",
    "游星欧米茄": "",
    "溟呼远响": "",
    "无言的闪光": "",
    "暗夜渔火": "",
    "雪花沉睡": "",
    "涟漪涤得残垣清": "",
    "我的高贵灵魂": "",
    "灭日": "",
    "你是黄金奖杯": "",
    "250霞龙": "",
    "250炎王龙 250炎王": "",
    "250钢龙": "",
    "270天彗龙 270天彗": "",
}

def extract_wiki_name(url):
    """从wiki.org URL提取怪物英文名"""
    fname = url.split('/')[-1]
    # 移除180px-前缀
    fname = re.sub(r'^\d+px-', '', fname)
    # 移除MHWilds-前缀
    fname = re.sub(r'^MHWilds-', '', fname)
    # 移除_Icon.webp
    fname = fname.replace('_Icon.webp', '')
    return fname.lower()

def find_zip_path(name_en):
    """根据英文名查找zip路径"""
    if name_en in zip_map:
        return zip_map[name_en]
    # 尝试模糊匹配
    for key, path in zip_map.items():
        if name_en in key or key in name_en:
            return path
    return None

def convert_monster_icons():
    """转换怪物图标URL为zip内路径"""
    
    # 读取原始JSON
    with open(JSON_PATH, 'r', encoding='utf-8') as f:
        data = json.load(f)
    
    # 备份原始文件
    shutil.copy2(JSON_PATH, BACKUP_PATH)
    print(f'Backup created: {BACKUP_PATH}')
    
    stats = {
        'total': len(data),
        'converted': 0,
        'already_local': 0,
        'wiki_converted': 0,
        'kiranico_converted': 0,
        'name_map_converted': 0,
        'not_found': 0,
        'unchanged': 0,
    }
    
    not_found_monsters = []
    
    for monster_name, info in data.items():
        url = info.get('图标地址', '')
        
        # 如果已经是本地路径（不含http），跳过
        if url and not url.startswith('http'):
            stats['already_local'] += 1
            continue
        
        # 如果中文名在直接映射表中
        if monster_name in MONSTER_NAME_MAP:
            new_path = MONSTER_NAME_MAP[monster_name]
            if new_path:
                info['图标地址'] = new_path
                stats['name_map_converted'] += 1
                print(f'[NAME_MAP] {monster_name} -> {new_path}')
            else:
                # 明确设置为空字符串（表示无图标）
                info['图标地址'] = ""
                stats['unchanged'] += 1
            continue
        
        # 处理wiki.org URL
        if 'monsterhunterwiki.org' in url:
            en_name = extract_wiki_name(url)
            zip_path = find_zip_path(en_name)
            if zip_path:
                info['图标地址'] = zip_path
                stats['wiki_converted'] += 1
                print(f'[WIKI] {monster_name} ({en_name}) -> {zip_path}')
            else:
                not_found_monsters.append((monster_name, url, f'wiki:{en_name}'))
                stats['not_found'] += 1
            continue
        
        # 处理kiranico.net URL（em编号）
        if 'cdn.kiranico.net' in url:
            em_match = re.search(r'em(\d+)', url)
            if em_match:
                em_id = f"em{em_match.group(1)}"
                if em_id in EM_ID_MAP:
                    info['图标地址'] = EM_ID_MAP[em_id]
                    stats['kiranico_converted'] += 1
                    print(f'[KIRANICO] {monster_name} ({em_id}) -> {EM_ID_MAP[em_id]}')
                else:
                    not_found_monsters.append((monster_name, url, f'em:{em_id}'))
                    stats['not_found'] += 1
            else:
                not_found_monsters.append((monster_name, url, 'no_em_id'))
                stats['not_found'] += 1
            continue
        
        # 无URL且不在映射表中
        if not url:
            stats['unchanged'] += 1
    
    # 写入新的JSON
    with open(JSON_PATH, 'w', encoding='utf-8') as f:
        json.dump(data, f, ensure_ascii=False, indent=2)
    
    # 输出统计
    print('\n' + '='*60)
    print('Conversion Statistics:')
    print(f'  Total monsters: {stats["total"]}')
    print(f'  Already local: {stats["already_local"]}')
    print(f'  Wiki converted: {stats["wiki_converted"]}')
    print(f'  Kiranico converted: {stats["kiranico_converted"]}')
    print(f'  Name map converted: {stats["name_map_converted"]}')
    print(f'  Unchanged (no icon): {stats["unchanged"]}')
    print(f'  Not found: {stats["not_found"]}')
    print(f'  Total converted: {stats["wiki_converted"] + stats["kiranico_converted"] + stats["name_map_converted"]}')
    print('='*60)
    
    if not_found_monsters:
        print('\nNot Found Monsters:')
        for name, url, reason in not_found_monsters:
            print(f'  {name} ({reason}): {url}')
    
    return stats, not_found_monsters

if __name__ == '__main__':
    stats, not_found = convert_monster_icons()
    print('\nConversion complete!')
    print(f'Backup saved to: {BACKUP_PATH}')