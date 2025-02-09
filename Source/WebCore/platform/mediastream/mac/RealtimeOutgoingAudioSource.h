/*
 * Copyright (C) 2017 Apple Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted, provided that the following conditions
 * are required to be met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. AND ITS CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#if USE(LIBWEBRTC)

#include "AudioSampleDataSource.h"
#include "LibWebRTCMacros.h"
#include "MediaStreamTrackPrivate.h"
#include <webrtc/api/mediastreaminterface.h>
#include <wtf/ThreadSafeRefCounted.h>

namespace webrtc {
class AudioTrackInterface;
class AudioTrackSinkInterface;
}

namespace WebCore {

class RealtimeOutgoingAudioSource final : public ThreadSafeRefCounted<RealtimeOutgoingAudioSource>, public webrtc::AudioSourceInterface, private MediaStreamTrackPrivate::Observer {
public:
    static Ref<RealtimeOutgoingAudioSource> create(Ref<MediaStreamTrackPrivate>&& audioSource) { return adoptRef(*new RealtimeOutgoingAudioSource(WTFMove(audioSource))); }
    ~RealtimeOutgoingAudioSource() { stop(); }

    void stop();

    bool setSource(Ref<MediaStreamTrackPrivate>&&);
    MediaStreamTrackPrivate& source() const { return m_audioSource.get(); }

private:
    explicit RealtimeOutgoingAudioSource(Ref<MediaStreamTrackPrivate>&&);

    virtual void AddSink(webrtc::AudioTrackSinkInterface* sink) { m_sinks.append(sink); }
    virtual void RemoveSink(webrtc::AudioTrackSinkInterface* sink) { m_sinks.removeFirst(sink); }

    int AddRef() const final { ref(); return refCount(); }
    int Release() const final { deref(); return refCount(); }
    SourceState state() const final { return kLive; }
    bool remote() const final { return false; }
    void RegisterObserver(webrtc::ObserverInterface*) final { }
    void UnregisterObserver(webrtc::ObserverInterface*) final { }

    void sourceMutedChanged();
    void sourceEnabledChanged();
    void audioSamplesAvailable(const MediaTime&, const PlatformAudioData&, const AudioStreamDescription&, size_t);

    // MediaStreamTrackPrivate::Observer API
    void trackMutedChanged(MediaStreamTrackPrivate&) final { sourceMutedChanged(); }
    void trackEnabledChanged(MediaStreamTrackPrivate&) final { sourceEnabledChanged(); }
    void audioSamplesAvailable(MediaStreamTrackPrivate&, const MediaTime& mediaTime, const PlatformAudioData& data, const AudioStreamDescription& description, size_t sampleCount) final { audioSamplesAvailable(mediaTime, data, description, sampleCount); }
    void trackEnded(MediaStreamTrackPrivate&) final { }
    void trackSettingsChanged(MediaStreamTrackPrivate&) final { }

    void pullAudioData();

    void initializeConverter();

    Vector<webrtc::AudioTrackSinkInterface*> m_sinks;
    Ref<MediaStreamTrackPrivate> m_audioSource;
    Ref<AudioSampleDataSource> m_sampleConverter;
    CAAudioStreamDescription m_inputStreamDescription;
    CAAudioStreamDescription m_outputStreamDescription;

    Vector<uint8_t> m_audioBuffer;
    uint64_t m_readCount { 0 };
    uint64_t m_writeCount { 0 };
    bool m_muted { false };
    bool m_enabled { true };
};

} // namespace WebCore

#endif // USE(LIBWEBRTC)
