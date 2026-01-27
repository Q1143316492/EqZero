
import * as UE from 'ue';
import { blueprint, argv } from 'puerts';
import { GameService } from './Logic/GameService';

function DebugLog(msg: string) {
    console.warn(`[TS Main] ${msg}`);
}

// =========================================================
// 1. 全局存储 (Script Global)
// Key: GameInstance Name
// =========================================================
const ServiceMap = new Map<string, GameService>();

export function GetGameService(Context: UE.Object): GameService | undefined {
    const GI = UE.GameplayStatics.GetGameInstance(Context);
    return GI ? ServiceMap.get(GI.GetName()) : undefined;
}

// 目标蓝图
const BP_Path = '/Game/B_EqGameInstance.B_EqGameInstance_C';
const TargetUClass = UE.Class.Load(BP_Path);

if (TargetUClass) {
    const TargetJSClass = blueprint.tojs<typeof UE.GameInstance>(TargetUClass);

    // =========================================================
    // 2. Mixin: 仅负责清理 (Cleanup Only)
    // =========================================================
    interface EqGameInstanceMixin extends UE.GameInstance {}
    class EqGameInstanceMixin {
        ReceiveShutdown(): void {
            const uid = this.GetName();
            DebugLog(`<<< ReceiveShutdown (Name:${uid})`);
            
            const service = ServiceMap.get(uid);
            if (service) {
                service.Destroy();
                ServiceMap.delete(uid);
            }
        }
    }
    blueprint.mixin(TargetJSClass, EqGameInstanceMixin);
    DebugLog(`Mixin applied.`);

    // =========================================================
    // 3. 脚本入口立即初始化 (Entry Init)
    // C++ Init() -> Start("Main", Args) -> 此处执行
    // =========================================================
    
    // 封装初始化函数
    function RegisterServiceFor(GI: UE.GameInstance) {

        if (!GI) 
        {
            DebugLog("[Init] Error: GameInstance is null or undefined.");
            return;
        }
        
        const uid = GI.GetName();
        // 防止重复初始化 (虽然正常逻辑下每个 GI 只会 Init 一次)
        if (!ServiceMap.has(uid)) {
            DebugLog(`[Init] Creating Global Service for ${GI.GetName()} (Name:${uid})`);
            ServiceMap.set(uid, new GameService(GI));
        } else {
            DebugLog(`[Init] Service already exists for (Name:${uid})`);
        }
    }

    try {
        const passedGI = argv.getByName("GameInstance") as UE.GameInstance;
        if (passedGI) {
            RegisterServiceFor(passedGI);
        } else {
            DebugLog("[Init] Warning: Script started but 'GameInstance' argument is missing.");
        }
    } catch (e) {
        DebugLog(`[Init] Error: ${e}`);
    }

} else {
    console.error(`ERROR: Failed to load Blueprint Class: ${BP_Path}`);
}
