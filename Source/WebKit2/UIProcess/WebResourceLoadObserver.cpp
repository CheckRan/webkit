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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebResourceLoadObserver.h"

#include <WebCore/ResourceLoadStatisticsStore.h>
#include <WebCore/URL.h>
#include <wtf/CrossThreadCopier.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/RunLoop.h>
#include <wtf/WorkQueue.h>

namespace WebKit {

using namespace WebCore;

template<typename T> static inline String primaryDomain(const T& value)
{
    return ResourceLoadStatisticsStore::primaryDomain(value);
}

WebResourceLoadObserver& WebResourceLoadObserver::sharedObserver()
{
    static NeverDestroyed<WebResourceLoadObserver> WebResourceLoadObserver;
    return WebResourceLoadObserver;
}

void WebResourceLoadObserver::setStatisticsStore(Ref<ResourceLoadStatisticsStore>&& store)
{
    if (m_store && m_queue)
        m_queue = nullptr;
    m_store = WTFMove(store);
}

void WebResourceLoadObserver::setStatisticsQueue(Ref<WTF::WorkQueue>&& queue)
{
    ASSERT(!m_queue);
    m_queue = WTFMove(queue);
}

void WebResourceLoadObserver::clearInMemoryStore()
{
    if (!m_store)
        return;

    ASSERT(m_queue);
    m_queue->dispatch([this] {
        m_store->clearInMemory();
    });
}

void WebResourceLoadObserver::clearInMemoryAndPersistentStore()
{
    if (!m_store)
        return;

    ASSERT(m_queue);
    m_queue->dispatch([this] {
        m_store->clearInMemoryAndPersistent();
    });
}

void WebResourceLoadObserver::clearInMemoryAndPersistentStore(std::chrono::system_clock::time_point modifiedSince)
{
    // For now, be conservative and clear everything regardless of modifiedSince
    UNUSED_PARAM(modifiedSince);
    clearInMemoryAndPersistentStore();
}

void WebResourceLoadObserver::logUserInteraction(const URL& url)
{
    if (url.isBlankURL() || url.isEmpty())
        return;

    ASSERT(m_queue);
    m_queue->dispatch([this, primaryDomainString = primaryDomain(url).isolatedCopy()] {
        {
            auto locker = holdLock(m_store->statisticsLock());
            auto& statistics = m_store->ensureResourceStatisticsForPrimaryDomain(primaryDomainString);
            statistics.hadUserInteraction = true;
            statistics.mostRecentUserInteractionTime = WallTime::now();
        }

        m_store->fireShouldPartitionCookiesHandler({ primaryDomainString }, { }, false);
    });
}

void WebResourceLoadObserver::clearUserInteraction(const URL& url)
{
    if (url.isBlankURL() || url.isEmpty())
        return;

    auto locker = holdLock(m_store->statisticsLock());
    auto& statistics = m_store->ensureResourceStatisticsForPrimaryDomain(primaryDomain(url));

    statistics.hadUserInteraction = false;
    statistics.mostRecentUserInteractionTime = { };
}

bool WebResourceLoadObserver::hasHadUserInteraction(const URL& url)
{
    if (url.isBlankURL() || url.isEmpty())
        return false;

    auto locker = holdLock(m_store->statisticsLock());
    auto& statistics = m_store->ensureResourceStatisticsForPrimaryDomain(primaryDomain(url));

    return m_store->hasHadRecentUserInteraction(statistics);
}

void WebResourceLoadObserver::setPrevalentResource(const URL& url)
{
    if (url.isBlankURL() || url.isEmpty())
        return;

    auto locker = holdLock(m_store->statisticsLock());
    auto& statistics = m_store->ensureResourceStatisticsForPrimaryDomain(primaryDomain(url));

    statistics.isPrevalentResource = true;
}

bool WebResourceLoadObserver::isPrevalentResource(const URL& url)
{
    if (url.isBlankURL() || url.isEmpty())
        return false;

    auto locker = holdLock(m_store->statisticsLock());
    auto& statistics = m_store->ensureResourceStatisticsForPrimaryDomain(primaryDomain(url));

    return statistics.isPrevalentResource;
}

void WebResourceLoadObserver::clearPrevalentResource(const URL& url)
{
    if (url.isBlankURL() || url.isEmpty())
        return;

    auto locker = holdLock(m_store->statisticsLock());
    auto& statistics = m_store->ensureResourceStatisticsForPrimaryDomain(primaryDomain(url));

    statistics.isPrevalentResource = false;
}

void WebResourceLoadObserver::setGrandfathered(const URL& url, bool value)
{
    if (url.isBlankURL() || url.isEmpty())
        return;

    auto locker = holdLock(m_store->statisticsLock());
    auto& statistics = m_store->ensureResourceStatisticsForPrimaryDomain(primaryDomain(url));

    statistics.grandfathered = value;
}

bool WebResourceLoadObserver::isGrandfathered(const URL& url)
{
    if (url.isBlankURL() || url.isEmpty())
        return false;

    auto locker = holdLock(m_store->statisticsLock());
    auto& statistics = m_store->ensureResourceStatisticsForPrimaryDomain(primaryDomain(url));

    return statistics.grandfathered;
}

void WebResourceLoadObserver::setSubframeUnderTopFrameOrigin(const URL& subframe, const URL& topFrame)
{
    if (subframe.isBlankURL() || subframe.isEmpty() || topFrame.isBlankURL() || topFrame.isEmpty())
        return;

    ASSERT(m_queue);
    m_queue->dispatch([this, primaryTopFrameDomainString = primaryDomain(topFrame).isolatedCopy(), primarySubFrameDomainString = primaryDomain(subframe).isolatedCopy()] {
        auto locker = holdLock(m_store->statisticsLock());
        auto& statistics = m_store->ensureResourceStatisticsForPrimaryDomain(primarySubFrameDomainString);
        statistics.subframeUnderTopFrameOrigins.add(primaryTopFrameDomainString);
    });
}

void WebResourceLoadObserver::setSubresourceUnderTopFrameOrigin(const URL& subresource, const URL& topFrame)
{
    if (subresource.isBlankURL() || subresource.isEmpty() || topFrame.isBlankURL() || topFrame.isEmpty())
        return;

    ASSERT(m_queue);
    m_queue->dispatch([this, primaryTopFrameDomainString = primaryDomain(topFrame).isolatedCopy(), primarySubresourceDomainString = primaryDomain(subresource).isolatedCopy()] {
        auto locker = holdLock(m_store->statisticsLock());
        auto& statistics = m_store->ensureResourceStatisticsForPrimaryDomain(primarySubresourceDomainString);
        statistics.subresourceUnderTopFrameOrigins.add(primaryTopFrameDomainString);
    });
}

void WebResourceLoadObserver::setSubresourceUniqueRedirectTo(const URL& subresource, const URL& hostNameRedirectedTo)
{
    if (subresource.isBlankURL() || subresource.isEmpty() || hostNameRedirectedTo.isBlankURL() || hostNameRedirectedTo.isEmpty())
        return;

    ASSERT(m_queue);
    m_queue->dispatch([this, primaryRedirectDomainString = primaryDomain(hostNameRedirectedTo).isolatedCopy(), primarySubresourceDomainString = primaryDomain(subresource).isolatedCopy()] {
        auto locker = holdLock(m_store->statisticsLock());
        auto& statistics = m_store->ensureResourceStatisticsForPrimaryDomain(primarySubresourceDomainString);
        statistics.subresourceUniqueRedirectsTo.add(primaryRedirectDomainString);
    });
}

void WebResourceLoadObserver::setTimeToLiveUserInteraction(Seconds seconds)
{
    m_store->setTimeToLiveUserInteraction(seconds);
}

void WebResourceLoadObserver::setTimeToLiveCookiePartitionFree(Seconds seconds)
{
    m_store->setTimeToLiveCookiePartitionFree(seconds);
}

void WebResourceLoadObserver::setMinimumTimeBetweeenDataRecordsRemoval(Seconds seconds)
{
    m_store->setMinimumTimeBetweeenDataRecordsRemoval(seconds);
}

void WebResourceLoadObserver::setGrandfatheringTime(Seconds seconds)
{
    m_store->setMinimumTimeBetweeenDataRecordsRemoval(seconds);
}

void WebResourceLoadObserver::fireDataModificationHandler()
{
    // Helper function used by testing system. Should only be called from the main thread.
    ASSERT(RunLoop::isMain());
    m_queue->dispatch([this] {
        m_store->fireDataModificationHandler();
    });
}

void WebResourceLoadObserver::fireShouldPartitionCookiesHandler()
{
    // Helper function used by testing system. Should only be called from the main thread.
    ASSERT(RunLoop::isMain());
    m_queue->dispatch([this] {
        m_store->fireShouldPartitionCookiesHandler();
    });
}

void WebResourceLoadObserver::fireShouldPartitionCookiesHandler(const Vector<String>& domainsToRemove, const Vector<String>& domainsToAdd, bool clearFirst)
{
    // Helper function used by testing system. Should only be called from the main thread.
    ASSERT(RunLoop::isMain());
    m_queue->dispatch([this, domainsToRemove = CrossThreadCopier<Vector<String>>::copy(domainsToRemove), domainsToAdd = CrossThreadCopier<Vector<String>>::copy(domainsToAdd), clearFirst] {
        m_store->fireShouldPartitionCookiesHandler(domainsToRemove, domainsToAdd, clearFirst);
    });
}

void WebResourceLoadObserver::fireTelemetryHandler()
{
    // Helper function used by testing system. Should only be called from the main thread.
    ASSERT(RunLoop::isMain());
    m_store->fireTelemetryHandler();
}

} // namespace WebKit
