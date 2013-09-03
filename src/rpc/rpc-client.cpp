extern "C" {
#include <searpc-client.h>
#include <ccnet.h>

#include <searpc.h>
#include <seafile/seafile.h>
#include <seafile/seafile-object.h>

}

#include <QSocketNotifier>
#include <QtDebug>
#include "seafile-applet.h"
#include "configurator.h"

#include "local-repo.h"
#include "rpc-client.h"


namespace {

const char *kSeafileRpcService = "seafile-rpcserver";
const char *kCcnetRpcService = "ccnet-rpcserver";

} // namespace

#define toCStr(_s)   ((_s).isNull() ? NULL : (_s).toUtf8().data())

SeafileRpcClient::SeafileRpcClient()
      : async_client_(0),
        seafile_rpc_client_(0),
        ccnet_rpc_client_(0),
        socket_notifier_(0)
{
}

void SeafileRpcClient::connectDaemon()
{
    async_client_ = ccnet_client_new();

    const QString config_dir = seafApplet->configurator()->ccnetDir();
    if (ccnet_client_load_confdir(async_client_, toCStr(config_dir)) <  0) {
        seafApplet->errorAndExit(tr("failed to load ccnet config dir %1").arg(config_dir));
    }

    if (ccnet_client_connect_daemon(async_client_, CCNET_CLIENT_ASYNC) < 0) {
        return;
    }

    seafile_rpc_client_ = ccnet_create_async_rpc_client(async_client_, NULL, kSeafileRpcService);
    ccnet_rpc_client_ = ccnet_create_async_rpc_client(async_client_, NULL, kCcnetRpcService);

    socket_notifier_ = new QSocketNotifier(async_client_->connfd, QSocketNotifier::Read);
    connect(socket_notifier_, SIGNAL(activated(int)), this, SLOT(readConnfd()));

    qDebug("[Rpc Client] connected to daemon");
}

void SeafileRpcClient::readConnfd()
{
    socket_notifier_->setEnabled(false);
    if (ccnet_client_read_input(async_client_) <= 0) {
        return;
    }
    socket_notifier_->setEnabled(true);
}
