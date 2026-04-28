## 1. 代码修改

- [x] 1.1 移除 `CaptainCheckInModule::PushDanmuEvent` 中首次打卡路径的 `SaveProfileAsync` 调用
- [x] 1.2 保留重复打卡和非打卡路径的 `SaveProfileAsync` 调用
- [x] 1.3 确保 `RecordCheckinAsync` 正确调用 `SaveProfileToDb`
- [x] 1.4 修复 `RecordCheckinAsync` 内存/DB 不一致：只有 checkin_records INSERT 成功才更新内存 profile
- [x] 1.5 使用 move 语义优化 `RecordCheckinAsync` 中的 profile 拷贝

## 2. 编译验证

- [x] 2.1 MSBuild Release x64 编译通过（0 errors）
- [x] 2.2 验证所有路径逻辑正确

## 3. 测试验证

- [ ] 3.1 验证首次打卡只写入一次数据库
- [ ] 3.2 验证重复打卡仍然正确保存
- [ ] 3.3 验证非打卡消息仍然正确保存
- [ ] 3.4 验证 checkin_records INSERT 失败时内存不更新
- [ ] 3.5 验证并发打卡场景下数据一致性
