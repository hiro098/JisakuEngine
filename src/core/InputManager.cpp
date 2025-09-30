#include "core/InputManager.h"
using namespace DirectX;
using namespace jisaku;

void InputManager::OnKeyEvent(WPARAM vk, bool pressed) {
    m_curr[(int)vk] = pressed;
}
void InputManager::OnMouseMove(int dx, int dy) {
    if (m_captured) {
        // マウス中央固定時の移動量を無視する場合
        if (m_ignoreNextMouseMove) {
            m_ignoreNextMouseMove = false;
            return;
        }
        // 相対移動量を累積（フレーム間で蓄積）
        m_dx += dx; 
        m_dy += dy;
    }
}

void InputManager::OnRawMouseMove(int dx, int dy) {
    if (m_captured) {
        // Raw Inputでは中央固定による移動は発生しないため、無視フラグは不要
        // 相対移動量を累積（フレーム間で蓄積）
        m_dx += dx; 
        m_dy += dy;
    }
}
void InputManager::OnMouseButton(int button, bool pressed) {
    if (button >= 0 && button < 3) {
        m_mouseButtons[button] = pressed;
        // 右クリックでマウスキャプチャ切り替え
        if (button == 1) { // 右クリック
            SetMouseCapture(pressed);
        }
    }
}
void InputManager::OnWheel(int delta) {
    m_wheel += delta;
}

bool InputManager::IsDown(int vk) const {
    auto it = m_curr.find(vk); return it != m_curr.end() && it->second;
}
bool InputManager::IsPressed(int vk) const {
    bool c = IsDown(vk);
    auto it = m_prev.find(vk); bool p = (it != m_prev.end() && it->second);
    return (c && !p);
}
bool InputManager::IsReleased(int vk) const {
    bool c = IsDown(vk);
    auto it = m_prev.find(vk); bool p = (it != m_prev.end() && it->second);
    return (!c && p);
}

void InputManager::NewFrame() {
    m_prev = m_curr;
    // マウス移動量は使用後にリセットするため、ここではリセットしない
    m_wheel = 0;
}

void InputManager::UpdateCamera(float dt) {
    XMVECTOR forward = XMVector3Rotate(XMVectorSet(0, 0, 1, 0), m_camRotQ);
    XMVECTOR right   = XMVector3Rotate(XMVectorSet(1, 0, 0, 0), m_camRotQ);
    XMVECTOR up      = XMVector3Rotate(XMVectorSet(0, 1, 0, 0), m_camRotQ);

    float speed = 5.0f * dt;
    if (IsDown('W')) m_camPos = XMVectorAdd(m_camPos, XMVectorScale(forward, speed));
    if (IsDown('S')) m_camPos = XMVectorSubtract(m_camPos, XMVectorScale(forward, speed));
    if (IsDown('A')) m_camPos = XMVectorAdd(m_camPos, XMVectorScale(right, speed));
    if (IsDown('D')) m_camPos = XMVectorSubtract(m_camPos, XMVectorScale(right, speed));
    if (IsDown(VK_SPACE))    m_camPos = XMVectorAdd(m_camPos, XMVectorScale(up, speed));
    if (IsDown(VK_CONTROL))  m_camPos = XMVectorSubtract(m_camPos, XMVectorScale(up, speed));

    // マウスによる回転 (dx,dy) - キャプチャ時のみ
    if (m_captured) {
        float sensitivity = m_mouseSensitivity; // デルタタイム非依存の感度
        float yaw = -m_dx * sensitivity;
        float pitch = -m_dy * sensitivity;

        // ピッチ制限（上下90度）
        m_pitch += pitch;
        m_pitch = (std::max)(-XM_PIDIV2, (std::min)(XM_PIDIV2, m_pitch));

        // クォータニオン積で回転を更新
        if (yaw != 0) {
            XMVECTOR qYaw = XMQuaternionRotationAxis(XMVectorSet(0, 1, 0, 0), yaw);
            m_camRotQ = XMQuaternionMultiply(m_camRotQ, qYaw);
        }
        // ピッチはワールドX軸ではなく、現在のカメラのローカルX軸で回転させる
        XMVECTOR currentRight = XMVector3Rotate(XMVectorSet(1, 0, 0, 0), m_camRotQ);
        if (pitch != 0) {
            XMVECTOR qPitch = XMQuaternionRotationAxis(currentRight, pitch);
            m_camRotQ = XMQuaternionMultiply(m_camRotQ, qPitch);
        }
        m_camRotQ = XMQuaternionNormalize(m_camRotQ);
        
        // マウス移動量を使用後にリセット
        m_dx = 0; m_dy = 0;
    }
}

void InputManager::SetMouseCapture(bool capture) {
    m_captured = capture;
    m_centerMouse = capture; // キャプチャ時にマウスを中央に固定
}

bool InputManager::IsMouseDown(int button) const {
    if (button >= 0 && button < 3) {
        return m_mouseButtons[button];
    }
    return false;
}

void InputManager::IgnoreNextMouseMove() {
    m_ignoreNextMouseMove = true;
}


