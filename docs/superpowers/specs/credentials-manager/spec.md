# CredentialsManager - 凭证管理

## 概述

敏感信息（API Key、Access Key 等）的加密存储和解密读取模块。

## 功能

### 常量定义
```cpp
constexpr const char* FILE_MAGIC = "@MonsterOrderSecret@";
constexpr const char* SALT = "@M0nst3r$Alt@";
```

### 核心 API
```cpp
bool LoadCredentials();
std::string GetAPP_ID();
std::string GetACCESS_KEY_ID();
std::string GetACCESS_KEY_SECRET();
std::string GetMIMO_API_KEY();
```

## 存储格式

凭证文件使用简单加密：
- 文件头: `FILE_MAGIC`
- 加密方式: XOR with `SALT`
- 存储路径: 与可执行文件同目录

## 文件结构

- `CredentialsManager.h`: 接口声明
- `CredentialsManager.cpp`: 实现

## 安全说明

使用简单 XOR 加密，适用于中等安全需求场景。
