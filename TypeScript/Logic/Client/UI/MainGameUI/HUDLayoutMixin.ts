import * as UE from 'ue';
import { registerUIMixin } from '../UIMixinHelper';
import './AnimDebug';
import type { IAnimDebugMixin } from './AnimDebug';

/**
 * W_EqZHUDLayout 蓝图混入
 * HUD主界面布局
 */
interface HUDLayoutMixin extends UE.EqZeroCore.UserInterface.W_EqZHUDLayout.W_EqZHUDLayout_C {}
class HUDLayoutMixin {
    
    BP_OnActivated(): void {
        console.warn(`[HUDLayout] >>> BP_OnActivated called! Widget: ${this.GetName()}`);
        this.TestTsToBP();
    }

    BP_OnDeactivated(): void {
        console.warn(`[HUDLayout] >>> BP_OnDeactivated called! Widget: ${this.GetName()}`);
    }
}

registerUIMixin({
    blueprintPath: '/EqZeroCore/UserInterface/W_EqZHUDLayout.W_EqZHUDLayout_C',
    mixinClass: HUDLayoutMixin,
    debugName: 'HUDLayout'
});
