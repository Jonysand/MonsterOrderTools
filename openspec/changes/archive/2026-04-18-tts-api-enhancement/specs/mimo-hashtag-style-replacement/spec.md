## ADDED Requirements

### Requirement: Mimo #标签 转 <style> 标签

当 TTS engine 为 Mimo 时，弹幕文本中的成对 #标签 需替换为 <style> 标签

#### Scenario: 弹幕包含单个成对 #标签
- **WHEN** 弹幕文本为 "#唱歌#你是我心中最美的云彩"
- **THEN** TTS 请求体中 text 为 "<style>唱歌</style>你是我心中最美的云彩"

#### Scenario: 弹幕包含多个成对 #标签
- **WHEN** 弹幕文本为 "#唱歌#你是我心中#最美#的云彩"
- **THEN** TTS 请求体中 text 为 "<style>唱歌</style>你是我心中<style>最美</style>的云彩"

#### Scenario: 弹幕不包含 #标签
- **WHEN** 弹幕文本为 "这是一条普通弹幕"
- **THEN** TTS 请求体中 text 保持不变

#### Scenario: 弹幕包含不成对的 #标签
- **WHEN** 弹幕文本为 "#唱歌你是我心中最美的云彩"
- **THEN** TTS 请求体中 text 保持不变（不成对的标签不替换）
