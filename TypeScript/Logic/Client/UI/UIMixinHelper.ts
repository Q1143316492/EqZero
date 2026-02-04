import * as UE from 'ue';
import { blueprint } from 'puerts';

/**
 * UI蓝图混入辅助工具
 * 用于简化UI蓝图与TypeScript类的混入注册流程
 * 
 * 使用方式：
 * ```typescript
 * // 1. 定义interface继承合适的UE基类，提供this的类型支持
 * // 根据你的蓝图父类选择：
 * // - UE.Widget (最基础)
 * // - UE.CommonActivatableWidget (CommonUI激活组件)
 * // - UE.CommonUserWidget (CommonUI基础组件)
 * // - 或者具体的蓝图类型路径: UE.Game.UI.MyWidget.MyWidget_C
 * 
 * interface MyWidgetMixin extends UE.CommonActivatableWidget {}
 * 
 * // 2. 定义Mixin类，实现蓝图事件
 * class MyWidgetMixin {
 *     BP_OnActivated(): void {
 *         this.GetName(); // ✅ 类型正确，有智能提示
 *     }
 * }
 * 
 * // 3. 注册混入
 * registerUIMixin({
 *     blueprintPath: '/Game/UI/W_MyWidget.W_MyWidget_C',
 *     mixinClass: MyWidgetMixin,
 *     debugName: 'MyWidget'
 * });
 * ```
 */

export interface UIMixinConfig {
    /** 蓝图资源路径，例如: '/Game/UI/W_MyWidget.W_MyWidget_C' */
    blueprintPath: string;
    /** 混入的TypeScript类 */
    mixinClass: any;
    /** 调试日志名称（可选），用于日志输出 */
    debugName?: string;
}

/**
 * 注册UI蓝图混入
 * @param config 混入配置
 * 
 * @example
 * ```typescript
 * class MyWidgetMixin {
 *     BP_OnActivated(): void {
 *         console.log('Widget activated');
 *     }
 * }
 * 
 * registerUIMixin({
 *     blueprintPath: '/Game/UI/W_MyWidget.W_MyWidget_C',
 *     mixinClass: MyWidgetMixin,
 *     debugName: 'MyWidget'
 * });
 * ```
 */
export function registerUIMixin(config: UIMixinConfig): void {
    const { blueprintPath, mixinClass, debugName } = config;
    const logPrefix = debugName ? `[${debugName}Mixin]` : '[UIMixin]';
    
    // 加载蓝图类
    const TargetUClass = UE.Class.Load(blueprintPath);
    
    if (!TargetUClass) {
        console.error(`${logPrefix} ERROR: Failed to load Blueprint Class: ${blueprintPath}`);
        return;
    }
    
    // 转换为JS类（使用any避免复杂的类型推导）
    const TargetJSClass = blueprint.tojs(TargetUClass);
    
    // 应用混入
    blueprint.mixin(TargetJSClass, mixinClass);
    
    console.warn(`${logPrefix} Mixin applied to ${blueprintPath}`);
}

/**
 * 批量注册UI蓝图混入
 * @param configs 混入配置数组
 */
export function registerUIMixins(configs: UIMixinConfig[]): void {
    configs.forEach(config => registerUIMixin(config));
}
