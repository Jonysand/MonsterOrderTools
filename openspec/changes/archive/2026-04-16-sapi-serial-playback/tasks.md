## 1. 修改 ProcessPendingRequest 中 SAPI 处理

- [x] 1.1 修改 SAPI 请求处理，使用 `SPF_ASYNC | SPF_IS_XML` 异步播放
- [x] 1.2 设置 `SetNotifyCallbackFunction` 监听 `SPEI_STREAM_ENDED` 事件
- [x] 1.3 气泡回调在播放前触发

## 2. 添加 SAPI 播放完成回调处理

- [x] 2.1 实现 SAPI 播放完成回调函数
- [x] 2.2 在回调中标记请求为 Completed
- [x] 2.3 触发 ProcessAsyncTTS 处理下一个请求

## 3. 添加 SAPI 播放状态管理

- [x] 3.1 在 AsyncTTSRequest 中添加 `bool sapiStreamEnded` 标记
- [x] 3.2 ProcessPlayingState 中检查 SAPI 流结束标记

## 4. 测试验证

- [ ] 4.1 使用 SAPI 引擎测试打卡回复，验证串行播放
- [ ] 4.2 验证播放完成后下一个请求被正确处理