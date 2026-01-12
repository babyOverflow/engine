namespace core::event {

enum class KeyCode {
    None = 0,
    Space = 32,
    Quote = 39,   // '
    Comma = 44,   // ,
    Minus = 45,   // -
    Period = 46,  // .
    Slash = 47,   // /

    // 숫자 (Top row)
    D0 = 48,
    D1 = 49,
    D2 = 50,
    D3 = 51,
    D4 = 52,
    D5 = 53,
    D6 = 54,
    D7 = 55,
    D8 = 56,
    D9 = 57,

    Semicolon = 59,  // ;
    Equal = 61,      // =

    A = 65,
    B = 66,
    C = 67,
    D = 68,
    E = 69,
    F = 70,
    G = 71,
    H = 72,
    I = 73,
    J = 74,
    K = 75,
    L = 76,
    M = 77,
    N = 78,
    O = 79,
    P = 80,
    Q = 81,
    R = 82,
    S = 83,
    T = 84,
    U = 85,
    V = 86,
    W = 87,
    X = 88,
    Y = 89,
    Z = 90,

    Escape = 256,
    Enter,
    Tab,
    Backspace,
    Insert,
    Delete,
    Right,
    Left,
    Down,
    Up,
    PageUp,
    PageDown,
    Home,
    End,
    CapsLock,
    ScrollLock,
    NumLock,
    PrintScreen,
    Pause,
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
    F13,
    F14,
    F15,
    F16,
    F17,
    F18,
    F19,
    F20,

    // --- Keypad ---
    KP_0,
    KP_1,
    KP_2,
    KP_3,
    KP_4,
    KP_5,
    KP_6,
    KP_7,
    KP_8,
    KP_9,
    KP_Decimal,
    KP_Divide,
    KP_Multiply,
    KP_Subtract,
    KP_Add,
    KP_Enter,
    KP_Equal,

    // --- Modifiers ---
    // 왼쪽/오른쪽 구분은 게임에서 매우 중요합니다.
    LeftShift,
    LeftControl,
    LeftAlt,
    LeftSuper,  // Super is Windows key or Command key
    RightShift,
    RightControl,
    RightAlt,
    RightSuper,
    Menu,
};
struct KeyPressEvent {
    KeyCode keyCode;
};

struct KeyReleaseEvent {
    KeyCode keyCode;
};

struct CursorPosEvent {
    double x;
    double y;
};

struct ScrollEvent {
    double xoffset;
    double yoffset;
};
}  // namespace core::event