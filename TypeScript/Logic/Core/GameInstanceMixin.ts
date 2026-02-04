import * as UE from 'ue';
import { blueprint, argv } from 'puerts';
import { GameService } from '../GameService';

// =========================================================
// GameInstance Mixin Interface
// =========================================================
interface EqGameInstance extends UE.GameInstance {
    GameService?: GameService;
}

// =========================================================
// 导出 GetGameService 工具函数
// =========================================================
export function GetGameService(Context: UE.Object): GameService | undefined {
    const GI = UE.GameplayStatics.GetGameInstance(Context) as EqGameInstance;
    if (GI) {
        if (!GI.GameService) {
            console.warn(`[GetGameService] Lazy init service for ${GI.GetName()}`);
            GI.GameService = new GameService(GI);
        }
        return GI.GameService;
    }
    return undefined;
}

// =========================================================
// GameInstance Mixin - 模块加载时自动执行（仅执行一次）
// =========================================================
(function initializeGameInstanceMixin() {
    function DebugLog(msg: string) {
        console.warn(`[TS GameInstance] ${msg}`);
    }

    const BP_Path = '/Game/B_EqGameInstance.B_EqGameInstance_C';
    const TargetUClass = UE.Class.Load(BP_Path);

    if (!TargetUClass) {
        console.error(`ERROR: Failed to load Blueprint Class: ${BP_Path}`);
        return;
    }

    const TargetJSClass = blueprint.tojs<typeof UE.GameInstance>(TargetUClass);

    // =========================================================
    // Mixin: Lifecycle Management
    // =========================================================
    interface EqGameInstanceMixin extends UE.GameInstance {}
    class EqGameInstanceMixin {
        
        ReceiveShutdown(): void {
            const self = this as unknown as EqGameInstance;
            const uid = self.GetName();
            DebugLog(`<<< ReceiveShutdown (Name:${uid})`);
            
            if (self.GameService) {
                self.GameService.Destroy();
                self.GameService = undefined;
            }
        }

        OnNativeInputAction(InputTag: UE.GameplayTag, InputActionValue: UE.InputActionValue): void {
            const self = this as unknown as EqGameInstance;
            // DebugLog(`OnNativeInputAction (Name:${self.GetName()}, Tag:${InputTag.TagName})`);
        }
    }
    
    // Apply the mixin to the Blueprint class
    blueprint.mixin(TargetJSClass, EqGameInstanceMixin);
    DebugLog(`Mixin applied to ${BP_Path}`);

    // =========================================================
    // 脚本入口立即初始化 (Entry Init)
    // C++ Init() -> Start("Main", Args) -> 此处执行
    // =========================================================
    try {
        const passedGI = argv.getByName("GameInstance") as EqGameInstance;
        if (passedGI) {
            DebugLog(`[Init] Binding Service for ${passedGI.GetName()}`);
            
            // Initialization: Attach service directly to the instance
            if (!passedGI.GameService) {
                passedGI.GameService = new GameService(passedGI);
            } else {
                DebugLog(`[Init] Service already exists for ${passedGI.GetName()}`);
            }
        } else {
            DebugLog("[Init] Warning: Script started but 'GameInstance' argument is missing.");
        }
    } catch (e) {
        DebugLog(`[Init] Error: ${e}`);
    }
})();
