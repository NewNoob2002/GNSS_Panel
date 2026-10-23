# 页面 MVP 范本

复制此目录后按下列步骤迁移，不要整页重写现有控件与动画：

1. 列出 View 真正使用的字段，定义最小值类型。Model 继承 `page::Model<Value>`（其基类为 `DataNode`），不包含 `ModelUtils.h`，不保存 Status/System/Device 聚合快照。
2. 只经 `dp::<Domain>::subscribe/pull` 订阅所需领域并取得初值。示例只依赖 Recorder；每增加一个领域，都在测试中校验精确订阅清单。
3. `onEvent` 用对应 Helper 的 `receive` 校验发布来源与尺寸，再复制需要的字段。字段未变直接返回，改变后调用 `changed()`；不要保存 event、data_p 或临时字符串指针。
4. subject 由 Model 的基类创建和销毁，pointer 值只指向 Model 自有状态。使用 LVGL 9.6 的 `lv_subject_create/delete`，避免已弃用的 init/deinit 接口及 release 告警。
5. View 用普通 `lv_subject_add_observer` 绑定；回调立即渲染 pull 到的初值。View 独占控件写入，通过语义 getter 向 Page 提供输入目标。
6. Page 在 `onViewLoad` 创建 Model/View，在 `onViewWillAppear` 绑定，在 `onViewWillDisappear` 显式解绑。`onViewUnload` 先 `view.destroy()`（幂等解绑），再删除 Model，最后由 PageManager 删除页面根控件。不要把普通 observer 当作随控件自动删除的 observer。
7. Page 仅转发输入、导航与生命周期。命令用 `dp::<Domain>::notify`；范本的录制按钮通过 Recorder 发命令。页面内导航保持同步：`getManager()->push(page::name(PageId::SystemInfos)) == PageManager::RES_TYPE::OK`，不要改为 PageNavi 异步队列。
8. 保留原动画、焦点、确认时序与所有断言。参考 `app_mvp_test.cpp` 覆盖初值、相关字段刷新、精确订阅、解绑与卸载；再跑截图字节比对和资源检查。复制成实际页面后在 `PageList.inc` 登记，本范本本身只参与 Linux 主机编译。

StatusBar 的 `reported_recording` 与 Dialplate 的 `recording` 有不同含义：前者等待主控确认，后者立即反映录制请求。迁移其他页面时沿用原语义。
