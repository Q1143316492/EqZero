
import * as UE from 'ue';

// =========================================================
// 核心系统 Mixin - 模块加载时自动初始化
// =========================================================

// GameInstance Mixin - 必须首先加载
import './Logic/Core/GameInstanceMixin';
export { GetGameService } from './Logic/Core/GameInstanceMixin';

// UI Mixin - 加载UI蓝图混入
import './Logic/Client/UI/EqFrontEndMixin';
import './Logic/Client/UI/MainGameUI/HUDLayoutMixin';

// Cheat Mixin - 开发工具（Development Only）
import './Logic/Client/Command/Cheat';
