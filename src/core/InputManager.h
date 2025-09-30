#pragma once
#include <Windows.h>
#include <DirectXMath.h>
#include <unordered_map>

namespace jisaku {

class InputManager {
public:
        void OnKeyEvent(WPARAM vk, bool pressed);
        void OnMouseMove(int dx, int dy);
        void OnRawMouseMove(int dx, int dy); // Raw Input用
        void OnMouseButton(int button, bool pressed);
        void OnWheel(int delta);

    bool IsDown(int vk) const;
    bool IsPressed(int vk) const;
    bool IsReleased(int vk) const;

    void NewFrame();
    void UpdateCamera(float dt);

    DirectX::XMVECTOR GetCameraPosition() const { return m_camPos; }
    DirectX::XMVECTOR GetCameraRotationQ() const { return m_camRotQ; }
    
    // マウスキャプチャ制御
    void SetMouseCapture(bool capture);
    bool ShouldCenterMouse() const { return m_centerMouse; }
    
    // マウスボタン状態取得
    bool IsMouseDown(int button) const;
    
    // マウス中央固定時の移動量を無視する
    void IgnoreNextMouseMove();

    // マウス感度の取得・設定
    float GetMouseSensitivity() const { return m_mouseSensitivity; }
    void SetMouseSensitivity(float sensitivity) { m_mouseSensitivity = sensitivity; }

private:
    std::unordered_map<int, bool> m_curr, m_prev;
    int m_dx = 0, m_dy = 0, m_wheel = 0;
    bool m_mouseButtons[3] = {};
    bool m_captured = false;
    bool m_centerMouse = false; // マウスを中央に固定するフラグ
    bool m_ignoreNextMouseMove = false; // 次のマウス移動を無視するフラグ

    DirectX::XMVECTOR m_camPos = DirectX::XMVectorSet(0, 0, -5, 1);
    DirectX::XMVECTOR m_camRotQ = DirectX::XMQuaternionIdentity();
    float m_pitch = 0.0f; // ピッチ角度制限用
    float m_mouseSensitivity = 0.005f; // マウス感度（デルタタイム非依存）
};

} // namespace jisaku


