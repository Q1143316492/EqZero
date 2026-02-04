import * as UE from 'ue';
import { blueprint } from 'puerts';

// =========================================================
// TypeScript Cheats Mixin - 模块加载时自动执行（仅执行一次）
// 使用 IIFE 避免污染模块作用域
// =========================================================
(function initializeTSCheats() {
    function DebugLog(msg: string) {
        console.warn(`[TS Cheat] ${msg}`);
    }

    const TSCheats_BP_Path = '/Game/Development/B_EqTSCheats.B_EqTSCheats_C';
    const TSCheatsUClass = UE.Class.Load(TSCheats_BP_Path);

    if (TSCheatsUClass) {
        const TSCheatsJSClass = blueprint.tojs(TSCheatsUClass);

        class EqTSCheatsMixin {
            TSTest(): void {
                DebugLog(`TSTest called!`);
                console.log('TypeScript Cheat System is working!');
            }

            TSCommand(Command: string): void {
                DebugLog(`TSCommand: ${Command}`);
                // 在这里可以添加自定义逻辑，例如解析命令并执行
                
                // 示例：简单的命令解析
                if (Command === 'help') {
                    console.log('Available TS Commands:');
                    console.log('  help - Show this help message');
                    console.log('  info - Show system info');
                }
            }

            TSDebug(Message: string): void {
                DebugLog(`Debug: ${Message}`);
            }
        }

        blueprint.mixin(TSCheatsJSClass, EqTSCheatsMixin as any);
        DebugLog(`Mixin applied to ${TSCheats_BP_Path}`);
    } else {
        DebugLog(`Warning: Failed to load Blueprint Class: ${TSCheats_BP_Path}`);
    }
})();
