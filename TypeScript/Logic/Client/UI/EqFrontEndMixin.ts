import * as UE from 'ue';
import { blueprint } from 'puerts';

function DebugLog(msg: string) {
    console.warn(`[EqFrontEndMixin] ${msg}`);
}

// =========================================================
// UI Blueprint Mixin for W_EqFrontEnd
// /Game/UI/Menu/W_EqFrontEnd.W_EqFrontEnd_C
// =========================================================

// 1. 加载蓝图类
const BP_Path = '/Game/UI/Menu/W_EqFrontEnd.W_EqFrontEnd_C';
const TargetUClass = UE.Class.Load(BP_Path);

// 定义Mixin接口，继承蓝图类以获取类型提示
interface EqFrontEndMixin extends UE.Game.UI.Menu.W_EqFrontEnd.W_EqFrontEnd_C {}

// 定义Mixin类，覆盖或扩展蓝图方法
class EqFrontEndMixin {
    
    /**
     * 覆盖 BP_OnActivated 事件
     * 当Widget被激活时调用
     */
    BP_OnActivated(): void {
        DebugLog(`>>> 111 BP_OnActivated called! Widget: ${this.GetName()}`);
        
        // 可以在这里添加额外的初始化逻辑
        // 例如：播放动画、加载数据、设置初始状态等
    }

    /**
     * 覆盖 BP_GetDesiredFocusTarget 事件
     * 返回Widget激活时应该获得焦点的目标
     */
    BP_GetDesiredFocusTarget(): UE.Widget {
        DebugLog(`>>> BP_GetDesiredFocusTarget called!`);
        
        // 返回 StartGameButton 作为默认焦点目标
        // 如果你的蓝图有其他需要聚焦的按钮，可以在这里修改
        if (this.StartGameButton) {
            DebugLog(`    Returning StartGameButton as focus target`);
            return this.StartGameButton;
        }
        
        // 如果没有按钮，返回null让引擎自己处理
        return null as unknown as UE.Widget;
    }

    // =========================================================
    // 可以添加纯TS方法（蓝图不可见，但TS可以调用）
    // =========================================================
    
    /**
     * 自定义的TS方法示例
     */
    TsCustomMethod(): void {
        DebugLog(`TsCustomMethod called on ${this.GetName()}`);
    }
}

// 导出类型定义
export { EqFrontEndMixin };

// 存储Mixin后的类
let W_EqFrontEndWithMixin: ReturnType<typeof blueprint.mixin> | undefined;

if (TargetUClass) {
    // 2. 转换为JS类，以便获取类型提示
    const W_EqFrontEnd = blueprint.tojs<typeof UE.Game.UI.Menu.W_EqFrontEnd.W_EqFrontEnd_C>(TargetUClass);

    // 3. 应用Mixin到蓝图类
    // objectTakeByNative: true 表示UE管理对象生命周期（UI Widget推荐使用）
    W_EqFrontEndWithMixin = blueprint.mixin(W_EqFrontEnd, EqFrontEndMixin, {
        objectTakeByNative: true
    });

    DebugLog(`Mixin applied to ${BP_Path}`);

} else {
    console.error(`[EqFrontEndMixin] ERROR: Failed to load Blueprint Class: ${BP_Path}`);
}

export { W_EqFrontEndWithMixin };
