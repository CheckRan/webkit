/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <wtf/Function.h>
#include <wtf/ThreadSafeRefCounted.h>
#include <wtf/WorkQueue.h>
#include <wtf/text/WTFString.h>

#if USE(COCOA_EVENT_LOOP)
#include <dispatch/dispatch.h>
#include <wtf/DispatchPtr.h>
#endif

namespace WebCore {

class FileMonitor : public ThreadSafeRefCounted<FileMonitor> {
public:
    enum class FileChangeType {
        Modification,
        Removal
    };

    WEBCORE_EXPORT static Ref<FileMonitor> create(const String&, Ref<WorkQueue>&& handlerQueue, WTF::Function<void(FileChangeType)>&& modificationHandler);
    WEBCORE_EXPORT ~FileMonitor();

    WEBCORE_EXPORT void startMonitoring();
    WEBCORE_EXPORT void stopMonitoring();

private:
    FileMonitor(const String&, Ref<WorkQueue>&& handlerQueue, WTF::Function<void(FileChangeType)>&& modificationHandler);

    String m_path;
    WTF::Function<void(FileChangeType)> m_modificationHandler;
    Ref<WTF::WorkQueue> m_handlerQueue;

#if USE(COCOA_EVENT_LOOP)
    DispatchPtr<dispatch_source_t> m_platformMonitor;
#endif
};

} // namespace WebCore

