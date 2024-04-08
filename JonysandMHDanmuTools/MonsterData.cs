﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Text.RegularExpressions;

namespace JonysandMHDanmuTools
{
    internal class MonsterData
    {
        private Dictionary<string, string> ORDERRABLE_MONSTERS;
        private static MonsterData _Inst=null;

        // Singleton
        public static MonsterData GetInst()
        {
            if (_Inst != null)
                return _Inst;
            _Inst = new MonsterData();
            return _Inst;
        }

        public MonsterData()
        {
            // 为了最后只导出一个dll文件，代码里写死吧
            ORDERRABLE_MONSTERS = new Dictionary<string, string>{
                {@"\b小明\b","冥灯龙"},
                {@"\b王冥灯\b","冥灯龙"},
                {@"\b冥灯\b","冥灯龙"},
                {@"\b王小明\b","冥灯龙"},
                {@"\b赤龙\b","冥赤龙"},
                {@"\b冥赤\b","冥赤龙"},
                {@"\b普冰\b","冰呪龙"},
                {@"\b冰呪\b","冰呪龙"},
                {@"\b冰粥\b","冰呪龙"},
                {@"\b普通冰呪龙\b","冰呪龙"},
                {@"\b普通冰呪\b","冰呪龙"},
                {@"\b普通冰粥\b","冰呪龙"},
                {@"\b普通冰粥龙\b","冰呪龙"},
                {@"\b冰冰子\b","冰呪龙"},
                {@"\b冰咒\b","冰呪龙"},
                {@"\b普通冰咒龙\b","冰呪龙"},
                {@"\b普通冰咒\b","冰呪龙"},
                {@"\b冰咒龙\b","冰呪龙"},
                {@"\b王冰\b","历战王冰呪龙"},
                {@"\b王冰冰\b","历战王冰呪龙"},
                {@"\b王冰呪\b","历战王冰呪龙"},
                {@"\b王冰粥\b","历战王冰呪龙"},
                {@"\b历战王冰呪\b","历战王冰呪龙"},
                {@"\b历战王冰粥龙\b","历战王冰呪龙"},
                {@"\b历战王冰粥\b","历战王冰呪龙"},
                {@"\b王冰咒\b","历战王冰呪龙"},
                {@"\b历战王冰咒\b","历战王冰呪龙"},
                {@"\b历战王冰咒龙\b","历战王冰呪龙"},
                {@"\b冰牙\b","冰牙龙"},
                {@"\b白色答辩\b","冰牙龙"},
                {@"\b灵活的狗\b","冰牙龙"},
                {@"\b山里灵活的狗\b","冰牙龙"},
                {@"\b白色粪怪\b","冰牙龙"},
                {@"\b冰鱼\b","冰鱼龙"},
                {@"\b凶爪\b","凶爪龙"},
                {@"\b古代鹿\b","古代鹿首精"},
                {@"\b土砂\b","土砂龙"},
                {@"\b痹贼\b","大痹贼龙"},
                {@"\b大凶颚龙\b","大痹贼龙"},
                {@"\b凶颚龙\b","大痹贼龙"},
                {@"\b中分哥\b","大贼龙"},
                {@"\b大凶豺龙\b","大贼龙"},
                {@"\b凶豺龙\b","大贼龙"},
                {@"\b贼龙\b","大贼龙"},
                {@"\b弟弟龙\b","天地煌啼龙"},
                {@"\b地啼龙\b","天地煌啼龙"},
                {@"\b天地煌啼\b","天地煌啼龙"},
                {@"\b尸套\b","尸套龙"},
                {@"\b王尸套\b","尸套龙"},
                {@"\b历战王尸套龙\b","尸套龙"},
                {@"\b历战王尸套\b","尸套龙"},
                {@"\b肥宅\b","岩贼龙"},
                {@"\b恐暴\b","恐暴龙"},
                {@"\b丝瓜\b","恐暴龙"},
                {@"\b黄瓜\b","恐暴龙"},
                {@"\b金瓜\b","恐暴龙"},
                {@"\b惨爪\b","惨爪龙"},
                {@"\b惶怒\b","惶怒恐暴龙"},
                {@"\b酱瓜\b","惶怒恐暴龙"},
                {@"\b瓜瓜\b","惶怒恐暴龙"},
                {@"\b阿瓜\b","惶怒恐暴龙"},
                {@"\b战痕黑鸡\b","战痕黑狼鸟"},
                {@"\b历战黑狼鸟\b","战痕黑狼鸟"},
                {@"\b历战黑鸡\b","战痕黑狼鸟"},
                {@"\b战痕\b","战痕黑狼鸟"},
                {@"\b骚鸟\b","搔鸟"},
                {@"\b斩龙\b","斩龙"},
                {@"\b樱火\b","樱火龙"},
                {@"\b歼灭\b","歼世灭尽龙"},
                {@"\b歼咩\b","歼世灭尽龙"},
                {@"\b花嫁灭尽\b","歼世灭尽龙"},
                {@"\b花嫁灭尽龙\b","歼世灭尽龙"},
                {@"\b花嫁歼咩\b","歼世灭尽龙"},
                {@"\b花嫁歼灭\b","歼世灭尽龙"},
                {@"\b咩咩\b","歼世灭尽龙"},
                {@"\b毒妖\b","毒妖鸟"},
                {@"\b水妖\b","水妖鸟"},
                {@"\b泥鱼\b","泥鱼龙"},
                {@"\b浮眠\b","浮眠龙"},
                {@"\b眠浮\b","浮眠龙"},
                {@"\b眠浮龙\b","浮眠龙"},
                {@"\b浮空\b","浮空龙"},
                {@"\b溟波\b","溟波龙"},
                {@"\b历战溟波\b","溟波龙"},
                {@"\b历战溟波龙\b","溟波龙"},
                {@"\b普通溟波龙\b","溟波龙"},
                {@"\b普通溟波\b","溟波龙"},
                {@"\b冥波\b","溟波龙"},
                {@"\b历战冥波\b","溟波龙"},
                {@"\b历战冥波龙\b","溟波龙"},
                {@"\b普通冥波龙\b","溟波龙"},
                {@"\b普通冥波\b","溟波龙"},
                {@"\b冥波龙\b","溟波龙"},
                {@"\b王溟波\b","历战王溟波龙"},
                {@"\b历战王溟波\b","历战王溟波龙"},
                {@"\b历战王冥波\b","历战王溟波龙"},
                {@"\b王冥波\b","历战王溟波龙"},
                {@"\b激昂\b","激昂金狮子"},
                {@"\b超级赛亚人\b","激昂金狮子"},
                {@"\b赛亚人\b","激昂金狮子"},
                {@"\b超赛\b","激昂金狮子"},
                {@"\b拉詹很生气\b","激昂金狮子"},
                {@"\b奖杯\b","激昂金狮子"},
                {@"\b你是黄金奖杯\b","激昂金狮子"},
                {@"\bJ8\b","激昂金狮子"},
                {@"\b雄火龙\b","火龙"},
                {@"\b雄火\b","火龙"},
                {@"\b天空王者\b","火龙"},
                {@"\b天空亡者\b","火龙"},
                {@"\b灭尽\b","灭尽龙"},
                {@"\b上位咩咩\b","灭尽龙"},
                {@"\b上位灭尽龙\b","灭尽龙"},
                {@"\b上位灭尽\b","灭尽龙"},
                {@"\b上位咩咩子\b","灭尽龙"},
                {@"\b王灭尽\b","历战王灭尽龙"},
                {@"\b王咩\b","历战王灭尽龙"},
                {@"\b历战王咩咩子\b","历战王灭尽龙"},
                {@"\b王咩咩\b","历战王灭尽龙"},
                {@"\b炎妃\b","炎妃龙"},
                {@"\b蓝猫\b","炎妃龙"},
                {@"\b蓝色粪怪\b","炎妃龙"},
                {@"\b疯婆子\b","炎妃龙"},
                {@"\b蓝色答辩\b","炎妃龙"},
                {@"\b高贵灵魂\b","炎妃龙"},
                {@"\b炎王\b","炎王龙"},
                {@"\b灭日\b","炎王龙"},
                {@"\b红猫\b","炎王龙"},
                {@"\b炎喵\b","炎王龙"},
                {@"\b红喵\b","炎王龙"},
                {@"\b火喵\b","炎王龙"},
                {@"\b粉喵\b","炎王龙"},
                {@"\b火炎王\b","炎王龙"},
                {@"\b粉炎王\b","炎王龙"},
                {@"\b煌黑\b","煌黑龙"},
                {@"\b羊驼\b","煌黑龙"},
                {@"\b煌尼玛\b","煌黑龙"},
                {@"\b黄黑龙\b","煌黑龙"},
                {@"\b黄尼玛\b","煌黑龙"},
                {@"\b草泥马\b","煌黑龙"},
                {@"\b熔山\b","熔山龙"},
                {@"\b烤鱼\b","熔岩龙"},
                {@"\b爆锤\b","爆锤龙"},
                {@"\b姚明\b","爆锤龙"},
                {@"\b车轮滚滚\b","爆锤龙"},
                {@"\b暴锤\b","爆锤龙"},
                {@"\b暴锤龙\b","爆锤龙"},
                {@"\b爆鳞\b","爆鳞龙"},
                {@"\bB52\b","爆鳞龙"},
                {@"\b上位爆鳞龙\b","爆鳞龙"},
                {@"\b上位爆鳞\b","爆鳞龙"},
                {@"\b阿爆\b","爆鳞龙"},
                {@"\b狱狼\b","狱狼龙"},
                {@"\b狱娘\b","狱狼龙"},
                {@"\b黑狗\b","狱狼龙"},
                {@"\b猛爆\b","猛爆碎龙"},
                {@"\b萌宝\b","猛爆碎龙"},
                {@"\b爆碎\b","猛爆碎龙"},
                {@"\b萌宝碎龙\b","猛爆碎龙"},
                {@"\b猛牛\b","猛牛龙"},
                {@"\b牛牛\b","猛牛龙"},
                {@"\b痹毒\b","痹毒龙"},
                {@"\b战地记者\b","眩鸟"},
                {@"\b记者\b","眩鸟"},
                {@"\b硫斩\b","硫斩龙"},
                {@"\b流斩\b","硫斩龙"},
                {@"\b碎龙\b","碎龙"},
                {@"\b红莲\b","红莲爆鳞龙"},
                {@"\b红莲爆鳞\b","红莲爆鳞龙"},
                {@"\b红莲阿爆\b","红莲爆鳞龙"},
                {@"\b富婆\b","绚辉龙"},
                {@"\b渣渣辉\b","绚辉龙"},
                {@"\b绚辉\b","绚辉龙"},
                {@"\b苍火\b","苍火龙"},
                {@"\b蛮颚\b","蛮颚龙"},
                {@"\b角龙\b","角龙"},
                {@"\b贝\b","贝希摩斯"},
                {@"\b普贝\b","贝希摩斯"},
                {@"\b极贝\b","极贝"},
                {@"\b车龙\b","轰龙"},
                {@"\b车车车龙\b","轰龙"},
                {@"\b车车车\b","轰龙"},
                {@"\b车车龙\b","轰龙"},
                {@"\b轰爬爬\b","轰龙"},
                {@"\b迅喵\b","迅龙"},
                {@"\b金火\b","金火龙"},
                {@"\b辣酱\b","金狮子"},
                {@"\b普通金狮子\b","金狮子"},
                {@"\b钢钢子\b","钢龙"},
                {@"\b阿钢\b","钢龙"},
                {@"\b银火\b","银火龙"},
                {@"\b雌火\b","雌火龙"},
                {@"\b太太\b","雌火龙"},
                {@"\b雷狼\b","雷狼龙"},
                {@"\b雷娘\b","雷狼龙"},
                {@"\b雷狗\b","雷狼龙"},
                {@"\b雷颚\b","雷颚龙"},
                {@"\b雾瘴尸套\b","雾瘴尸套龙"},
                {@"\b花嫁尸套龙\b","雾瘴尸套龙"},
                {@"\b花嫁尸套\b","雾瘴尸套龙"},
                {@"\b霜牙\b","霜刃冰牙龙"},
                {@"\b霜刃冰牙\b","霜刃冰牙龙"},
                {@"\b霜冰\b","霜刃冰牙龙"},
                {@"\b霜漂\b","霜翼风漂龙"},
                {@"\b霜飘\b","霜翼风漂龙"},
                {@"\b霜翼风漂\b","霜翼风漂龙"},
                {@"\b霜翼风飘龙\b","霜翼风漂龙"},
                {@"\b霜翼风飘\b","霜翼风漂龙"},
                {@"\b风漂\b","风漂龙"},
                {@"\b风飘\b","风漂龙"},
                {@"\b风飘龙\b","风漂龙"},
                {@"\b飞雷\b","飞雷龙"},
                {@"\b骨锤\b","骨锤龙"},
                {@"\b鹿首精\b","鹿首精"},
                {@"\b电驴\b","麒麟"},
                {@"\b断角麒麟\b","麒麟"},
                {@"\b彩虹小马\b","麒麟"},
                {@"\b小马\b","麒麟"},
                {@"\b黑鸡\b","黑狼鸟"},
                {@"\b黑疯子\b","黑狼鸟"},
                {@"\b黑狼\b","黑狼鸟"},
                {@"\b黑角\b","黑角龙"},
                {@"\b黑轰\b","黑轰龙"},
                {@"\b白轰\b","黑轰龙"},
                {@"\b米拉\b","黑龙"},
                {@"\b米拉小姐\b","黑龙"},
                {@"\b嗨龙\b","黑龙"},
                {@"\b米拉波雷亚斯\b","黑龙"},
                {@"\b原爵\b","原初形态爵银龙"},
                {@"\b烈祸爵银\b","原初形态爵银龙"},
                {@"\b烈祸爵\b","原初形态爵银龙"},
                {@"\b原初爵银\b","原初形态爵银龙"},
                {@"\b原初爵\b","原初形态爵银龙"},
                {@"\b人鱼\b","人鱼龙"},
                {@"\b怪异人鱼\b","人鱼龙"},
                {@"\b怪异化人鱼龙\b","人鱼龙"},
                {@"\b怪异化人鱼\b","人鱼龙"},
                {@"\b怪异人鱼龙\b","人鱼龙"},
                {@"\b300人鱼\b","人鱼龙"},
                {@"\b300人鱼龙\b","人鱼龙"},
                {@"\b唐伞妖怪\b","伞鸟"},
                {@"\b怪异伞鸟\b","伞鸟"},
                {@"\b怪异化伞鸟\b","伞鸟"},
                {@"\b300伞鸟\b","伞鸟"},
                {@"\b冥渊\b","冥渊龙"},
                {@"\b冰人鱼\b","冰人鱼龙"},
                {@"\b冰狼\b","冰狼龙"},
                {@"\b加鲁鲁\b","冰狼龙"},
                {@"\b兽人加鲁鲁\b","冰狼龙"},
                {@"\b刚缠\b","刚缠兽"},
                {@"\b千刃\b","千刃龙"},
                {@"\b千刃鸡\b","千刃龙"},
                {@"\b明日香\b","嗟怨震天怨虎龙"},
                {@"\b霸天虎\b","嗟怨震天怨虎龙"},
                {@"\b大胖虎\b","嗟怨震天怨虎龙"},
                {@"\b独眼胖虎\b","嗟怨震天怨虎龙"},
                {@"\b震天虎\b","嗟怨震天怨虎龙"},
                {@"\b盾蟹\b","大名盾蟹"},
                {@"\b天廻\b","天廻龙"},
                {@"\b天回\b","天廻龙"},
                {@"\b天回龙\b","天廻龙"},
                {@"\b白丝\b","天廻龙"},
                {@"\b天狗\b","天狗兽"},
                {@"\b舔狗\b","天狗兽"},
                {@"\b舔狗兽\b","天狗兽"},
                {@"\b夫鲁夫鲁\b","奇怪龙"},
                {@"\b奇怪牛牛\b","奇怪龙"},
                {@"\b妃蜘蛛\b","妃蜘蛛"},
                {@"\b镰蟹\b","将军镰蟹"},
                {@"\b岚妹\b","岚龙"},
                {@"\b岩龙\b","岩龙"},
                {@"\b胖虎\b","怨虎龙"},
                {@"\b胖虎龙\b","怨虎龙"},
                {@"\b怪异克服天廻\b","怪异克服天廻龙"},
                {@"\b怪克天廻龙\b","怪异克服天廻龙"},
                {@"\b怪克天廻\b","怪异克服天廻龙"},
                {@"\b怪异克服天回\b","怪异克服天廻龙"},
                {@"\b怪克天回龙\b","怪异克服天廻龙"},
                {@"\b怪克天回\b","怪异克服天廻龙"},
                {@"\b300天回\b","怪异克服天廻龙"},
                {@"\b300天廻\b","怪异克服天廻龙"},
                {@"\b270天回 270天廻 270天廻龙 270天回龙\b","270天回 270天廻 270天廻龙 270天回龙"},
                {@"\b怪异克服天彗\b","怪异克服天彗龙"},
                {@"\b怪克天彗龙\b","怪异克服天彗龙"},
                {@"\b怪克天彗\b","怪异克服天彗龙"},
                {@"\b300天彗\b","怪异克服天彗龙"},
                {@"\b300天彗龙\b","怪异克服天彗龙"},
                {@"\b270天彗龙 270天彗\b","270天彗龙 270天彗"},
                {@"\b怪异克服炎王\b","怪异克服炎王龙"},
                {@"\b怪克炎王龙\b","怪异克服炎王龙"},
                {@"\b怪克炎王\b","怪异克服炎王龙"},
                {@"\b300炎王\b","怪异克服炎王龙"},
                {@"\b300炎王龙\b","怪异克服炎王龙"},
                {@"\b250炎王龙 250炎王\b","250炎王龙 250炎王"},
                {@"\b怪克钢龙\b","怪异克服钢龙"},
                {@"\b300钢龙\b","怪异克服钢龙"},
                {@"\b250钢龙\b","250钢龙"},
                {@"\b怪克霞龙\b","怪异克服霞龙"},
                {@"\b300霞龙\b","怪异克服霞龙"},
                {@"\b250霞龙\b","250霞龙"},
                {@"\b普通月迅\b","月迅龙"},
                {@"\b月迅\b","月迅龙"},
                {@"\b烈祸月迅\b","烈祸月迅龙"},
                {@"\b茶几\b","棘茶龙"},
                {@"\b茶几龙\b","棘茶龙"},
                {@"\b棘茶\b","棘茶龙"},
                {@"\b烈祸棘茶\b","烈祸棘茶龙"},
                {@"\b烈祸棘\b","烈祸棘茶龙"},
                {@"\b怪异克服棘龙\b","棘龙"},
                {@"\b怪克棘龙\b","棘龙"},
                {@"\b毒狗\b","毒狗龙王"},
                {@"\b怪异克服毒狗龙王\b","毒狗龙王"},
                {@"\b怪克毒狗龙王\b","毒狗龙王"},
                {@"\b怪异克服毒狗\b","毒狗龙王"},
                {@"\b怪克毒狗\b","毒狗龙王"},
                {@"\b怪异毒狗\b","毒狗龙王"},
                {@"\b怪异毒狗龙\b","毒狗龙王"},
                {@"\b怪异毒狗龙王\b","毒狗龙王"},
                {@"\b香蕉\b","水兽"},
                {@"\b怪异克服水兽\b","水兽"},
                {@"\b怪克水兽\b","水兽"},
                {@"\b怪异水兽\b","水兽"},
                {@"\b力士\b","河童蛙"},
                {@"\b蛙蛙\b","河童蛙"},
                {@"\b泡狐\b","泡狐龙"},
                {@"\b泡泡\b","泡狐龙"},
                {@"\b泥翁\b","泥翁龙"},
                {@"\b混沌黑蚀\b","混沌黑蚀龙"},
                {@"\b奥利奥\b","混沌黑蚀龙"},
                {@"\b混沌黑丝\b","混沌黑蚀龙"},
                {@"\b红蜘蛛\b","炽妃蜘蛛"},
                {@"\b焰狐\b","焰狐龙"},
                {@"\b紫色答辩\b","焰狐龙"},
                {@"\b熔翁\b","熔翁龙"},
                {@"\b爵银\b","爵银龙"},
                {@"\b电龙\b","电龙"},
                {@"\b白兔\b","白兔兽"},
                {@"\b百龙渊源\b","百龙渊源雷神龙"},
                {@"\b百雷\b","百龙渊源雷神龙"},
                {@"\b渊雷\b","百龙渊源雷神龙"},
                {@"\b眠狗\b","眠狗龙王"},
                {@"\b眠狗龙\b","眠狗龙王"},
                {@"\b天彗\b","神秘红光天彗龙"},
                {@"\b天彗龙\b","神秘红光天彗龙"},
                {@"\b绯天狗\b","绯天狗兽"},
                {@"\b赤甲\b","赤甲兽"},
                {@"\b镰鼬\b","镰鼬龙王"},
                {@"\b镰鼬龙\b","镰鼬龙王"},
                {@"\b雪鬼\b","雪鬼兽"},
                {@"\b双刀哥\b","雪鬼兽"},
                {@"\b双刀侠\b","雪鬼兽"},
                {@"\b雷神\b","雷神龙"},
                {@"\b霞龙\b","霞龙"},
                {@"\b霸主泡狐\b","霸主泡狐龙"},
                {@"\b霸主雄火龙\b","霸主火龙"},
                {@"\b霸主角龙\b","霸主角龙"},
                {@"\b霸主雌火\b","霸主雌火龙"},
                {@"\b霸主太太\b","霸主雌火龙"},
                {@"\b霸主雷狼\b","霸主雷狼龙"},
                {@"\b霸主雷娘\b","霸主雷狼龙"},
                {@"\b霸主雷狗\b","霸主雷狼龙"},
                {@"\b霸主青熊\b","霸主青熊兽"},
                {@"\b青熊\b","青熊兽"},
                {@"\b风神\b","风神龙"},
                {@"\b黑蚀\b","黑蚀龙"},
                {@"\b黑丝\b","黑蚀龙"},
                {@"\b嗨丝\b","黑蚀龙"},
            };
            
        }

        public string GetMatchedMonsterName(string inputText)
        {
            foreach (var item in ORDERRABLE_MONSTERS)
            {
                Match match = Regex.Match(inputText, item.Key);
                if (match.Success)
                    return item.Value;
            }
            return "";
        }
    }
}
