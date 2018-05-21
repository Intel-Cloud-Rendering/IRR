// Copyright 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#ifndef IRRCONTROLHANDLER_H
#define IRRCONTROLHANDLER_H

#include "generated/IrrControl.h"

namespace IntelCloudRendering {
    class IrrControlHandler : virtual public IrrControlIf {
    public:
        IrrControlHandler();

        ~IrrControlHandler();

        void ping();

        int32_t startStream(const StreamInfo &info);

        void stopStream();

        int32_t restartStream(const StreamInfo &info);
    };
}

#endif /* IRRCONTROLHANDLER_H */

