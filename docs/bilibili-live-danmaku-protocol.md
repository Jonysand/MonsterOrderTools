# 哔哩哔哩直播弹幕协议文档

> 来源：https://open.bilibili.com/doc/4/c7b549c9-d5d6-0331-b510-465ed4107e01
> 获取时间：2026-04-13

## 弹幕消息 (OPEN_LIVEROOM_DM)

### 返回字段

| 字段名 | 类型 | 描述 |
|--------|------|------|
| uname | string | 用户昵称 |
| open_id | string | 用户唯一标识 |
| union_id | string | 用户在同一开发者下的唯一标识（默认空） |
| uface | string | 用户头像 |
| timestamp | int64 | 弹幕发送时间秒级时间戳 |
| room_id | int64 | 弹幕接收的直播间 |
| msg | string | 弹幕内容 |
| msg_id | string | 消息唯一id |
| guard_level | int64 | 对应房间大航海等级 |
| fans_medal_wearing_status | bool | 该房间粉丝勋章佩戴情况 |
| fans_medal_name | string | 粉丝勋章名 |
| fans_medal_level | int64 | 对应房间勋章信息 |
| emoji_img_url | string | 表情包图片地址 |
| dm_type | int64 | 弹幕类型 0：普通弹幕 1：表情包弹幕 |
| glory_level | int | 直播荣耀等级 |
| reply_open_id | string | 被at用户唯一标识 |
| reply_uname | string | 被at的用户昵称 |
| is_admin | int | 发送弹幕的用户是否是房管，取值0或1 |

### guard_level 等级说明

| 值 | 等级 |
|----|------|
| 0 | 未上大航海 |
| 1 | 总督 |
| 2 | 提督 |
| 3 | 舰长 |

### 参考JSON

```json
{
    "cmd":"OPEN_LIVEROOM_DM",
    "data":{
        "room_id":1,
        "open_id":"39b8fedb-60a5-4e29-ac75-b16955f7e632",
        "union_id":"U_05ad57b6655a44528cb95a892c491232",
        "uname":"",
        "msg":"",
        "msg_id":"",
        "fans_medal_level":0,
        "fans_medal_name":"粉丝勋章名",
        "fans_medal_wearing_status": true,
        "guard_level":0,
        "timestamp":0,
        "uface":"",
        "emoji_img_url": "",
        "dm_type": 0,
        "glory_level": 39,
        "reply_open_id": "39b8fedb-60a5-4e29-ac75-b16955f7e632",
        "reply_uname": "",
        "is_admin":1
    }
}
```

## 礼物消息 (OPEN_LIVEROOM_SEND_GIFT)

### 参考JSON

```json
{
    "cmd":"OPEN_LIVEROOM_SEND_GIFT",
    "data":{
        "room_id":1,
        "open_id":"39b8fedb-60a5-4e29-ac75-b16955f7e632",
        "uname":"",
        "gift_id":0,
        "gift_name":"",
        "gift_num":0,
        "price":0,
        "paid":false,
        "fans_medal_level":0,
        "guard_level":0,
        "timestamp":0,
        "msg_id":""
    }
}
```

## 付费留言/SC (OPEN_LIVEROOM_SUPER_CHAT)

### 参考JSON

```json
{
    "cmd":"OPEN_LIVEROOM_SUPER_CHAT",
    "data":{
        "room_id":1,
        "open_id":"39b8fedb-60a5-4e29-ac75-b16955f7e632",
        "uname":"",
        "message_id":0,
        "message":"",
        "rmb":0,
        "timestamp":0,
        "guard_level": 2,
        "fans_medal_level": 26,
        "fans_medal_name": "aw4ifC",
        "fans_medal_wearing_status": true
    }
}
```

## 大航海/上舰 (OPEN_LIVEROOM_GUARD)

### 参考JSON

```json
{
    "cmd": "OPEN_LIVEROOM_GUARD",
    "data": {
        "user_info": {
            "open_id":"39b8fedb-60a5-4e29-ac75-b16955f7e632",
            "uname":"",
            "uface": ""
        },
        "guard_level": 3,
        "guard_num": 1,
        "guard_unit": "月",
        "price": 198000,
        "fans_medal_level": 24,
        "fans_medal_name": "aw4ifC",
        "fans_medal_wearing_status": false,
        "timestamp": 1653555128,
        "room_id": 460695,
        "msg_id":""
    }
}
```

## 进入房间 (OPEN_LIVEROOM_LIVE_ROOM_ENTER)

### 参考JSON

```json
{
    "cmd":"OPEN_PLATFORM_LIVE_ROOM_ENTER",
    "data":{
        "room_id":1,
        "uface":"",
        "uname":"",
        "open_id":"39b8fedb-60a5-4e29-ac75-b16955f7e632",
        "timestamp":0
    }
}
```

## 代码对应关系

### DanmuProcessor.cpp 字段映射

| B站字段 | 代码字段 | 处理逻辑 |
|---------|---------|---------|
| open_id | userId | 优先使用 open_id，兼容 uid |
| uname | userName | 直接使用 |
| msg | message | 直接使用 |
| timestamp | timestamp | 优先使用 timestamp，兼容 send_time |
| guard_level | guardLevel | != 0 即表示已上舰 |
| fans_medal_wearing_status | hasMedal | 直接使用 |
| fans_medal_level | medalLevel | 直接使用 |

### TextToSpeech.cpp 字段映射

| B站字段 | 用途 |
|---------|------|
| uname | 语音播报用户名 |
| msg | 语音播报内容 |
| guard_level | 仅播报指定舰长等级 |
| fans_medal_wearing_status | 仅播报佩戴勋章用户 |

## 注意事项

1. **字段兼容性**：代码中使用 `open_id` 优先，`uid` 回退；`timestamp` 优先，`send_time` 回退
2. **舰长判断**：使用 `guard_level != 0` 判断是否上舰（总督/提督/舰长均触发）
3. **TTS处理**：弹幕 "签到"/"打卡" 会被特殊处理，不会加入普通 TTS 队列
