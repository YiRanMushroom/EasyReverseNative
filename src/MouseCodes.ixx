export module MouseCodes;

import std.compat;

namespace Mouse {
    export using MouseCode = uint16_t;

    export enum : uint16_t {
        Button0 = 0,
        Button1, Button2, Button3, Button4, Button5, Button6, Button7,
        ButtonLast = Button7,
        ButtonLeft = Button0,
        ButtonRight = Button1,
        ButtonMiddle = Button2
    };
}
