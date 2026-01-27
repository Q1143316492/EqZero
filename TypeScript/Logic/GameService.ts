import * as UE from 'ue';

function DebugLog(msg: string) {
    console.warn(`[GameService] ${msg}`);
}

// =========================================================
// 1. 纯净的业务服务类
// =========================================================
export class GameService {
    private static _NEXT_ID = 1000;
    public readonly UniqueId: number;

    private ownerName: string;
    
    constructor(owner: UE.GameInstance) {
        this.UniqueId = GameService._NEXT_ID++;
        this.ownerName = owner.GetName();
        DebugLog(`>>> CREATED [UID:${this.UniqueId}] for ${this.ownerName}`);
    }

    Destroy() {
        DebugLog(`<<< DESTROYED [UID:${this.UniqueId}] for ${this.ownerName}`);
    }

    public DoSomething() {
        console.log(`[UID:${this.UniqueId}] Doing work...`);
    }
}
