#pragma once

namespace basic {

class zero {
public:
    zero() {};

    inline operator double() const
    {
        return 0;
    };

};

static const zero Zero;

}
