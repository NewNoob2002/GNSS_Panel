#pragma once

class PageManager;

/** 按显式页面清单组装应用，不使用静态对象自注册。 */
class AppFactory {
  public:
    /**
     * 在应用线程按清单顺序创建并安装页面，按能力跳过条件页面。
     * @param manager 接收页面所有权的管理器，寿命须覆盖全部已安装页面。
     * @return 全部适用页面安装成功时返回 true；失败实例立即释放，已安装实例仍由管理器持有。
     */
    static bool installAll(PageManager& manager);
};
