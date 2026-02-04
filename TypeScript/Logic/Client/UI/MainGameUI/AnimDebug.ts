import * as UE from 'ue';
import { registerUIMixin } from '../UIMixinHelper';


// /EqZeroCore/UserInterface/Debug/W_EqZAnimDebug.W_EqZAnimDebug
/**
 * W_EqZAnimDebug 蓝图混入
 * HUD主界面布局中的一个动画调试面板
 */

// 导出接口，供其他文件使用类型定义
export interface IAnimDebugMixin {
    UpdateAnimDebugInfo(animData: any): void;
    ShowDebugPanel(visible: boolean): void;
}

interface AnimDebugMixin extends UE.EqZeroCore.UserInterface.Debug.W_EqZAnimDebug.W_EqZAnimDebug_C {}
class AnimDebugMixin {
    
    BP_OnActivated(): void {
    
    }

    BP_OnDeactivated(): void {
    }

    public ShowDebugPanel(visible: boolean): void {
        console.warn(`[AnimDebug] ShowDebugPanel: ${visible}`);
        this.SetVisibility(visible ? UE.ESlateVisibility.HitTestInvisible : UE.ESlateVisibility.Collapsed);
    }
}

registerUIMixin({
    blueprintPath: '/EqZeroCore/UserInterface/Debug/W_EqZAnimDebug.W_EqZAnimDebug_C',
    mixinClass: AnimDebugMixin,
    debugName: 'AnimDebug'
});
