import 'dart:async';
import 'dart:io';
import 'dart:typed_data';

const MSG_UINT32_SIZE = 4;

//#define SS_MSG_HEAD_SIGN    0x11223344
const SS_MSG_HEAD_SIGN = 0x11223344;

//typedef struct {
//    uint32_t    head_sign;
//    uint32_t    body_len;
//} ss_msg_t;
//
const SS_MSG_HEAD_SIGN_OFFSET = 0;
const SS_MSG_HEAD_SIGN_SIZE = MSG_UINT32_SIZE;
const SS_MSG_BODY_LEN_OFFSET = SS_MSG_HEAD_SIGN_OFFSET + SS_MSG_HEAD_SIGN_SIZE;
const SS_MSG_BODY_LEN_SIZE = MSG_UINT32_SIZE;
const SS_MSG_SIZE = SS_MSG_HEAD_SIGN_SIZE + SS_MSG_BODY_LEN_SIZE;

// #define EMB_ID_NOT_USE    0
// #define EMB_ID_BROADCAST  1
const EMB_ID_NOT_USE = 0;
const EMB_ID_BROADCAST = 1;

//typedef enum {
//    // Common
//    EMB_MSG_TYPE_PUBLISH     = 100,
//    // Client -> Broker
//    //EMB_MSG_TYPE_CONNECT     = 200,
//    EMB_MSG_TYPE_SUBSCRIBE   = 201,
//    EMB_MSG_TYPE_UNSUBSCRIBE = 202,
//    //EMB_MSG_TYPE_DISCONNECT  = 203,
//    //EMB_MSG_TYPE_PINGREQ     = 204,
//    // Broker -> Client
//    //EMB_MSG_TYPE_CONNACK     = 300,
//    EMB_MSG_TYPE_SUBACK      = 301,
//    EMB_MSG_TYPE_UNSUBACK    = 302,
//    //EMB_MSG_TYPE_PINGRESP    = 304
//} emb_msg_type_t;
const EMB_MSG_TYPE_PUBLISH = 100;
const EMB_MSG_TYPE_SUBSCRIBE = 201;
const EMB_MSG_TYPE_UNSUBSCRIBE = 202;
const EMB_MSG_TYPE_SUBACK = 301;
const EMB_MSG_TYPE_UNSUBACK = 302;

//
//#define EMB_MSG_TOPIC_MAX       128
//#define EMB_MSG_DATA_MAX        256
const EMB_MSG_TOPIC_MAX = 128;
const EMB_MSG_DATA_MAX = 256;

//typedef struct {
//    emb_msg_type_t  type;
//    int32_t         len;
//} emb_msg_header_t;
const EMB_MSG_HEADER_TYPE_OFFSET =
    SS_MSG_BODY_LEN_OFFSET + SS_MSG_BODY_LEN_SIZE;
const EMB_MSG_HEADER_TYPE_SIZE = MSG_UINT32_SIZE;
const EMB_MSG_HEADER_LEN_OFFSET =
    EMB_MSG_HEADER_TYPE_OFFSET + EMB_MSG_HEADER_TYPE_SIZE;
const EMB_MSG_HEADER_LEN_SIZE = MSG_UINT32_SIZE;
const EMB_MSG_HEADER_SIZE = EMB_MSG_HEADER_TYPE_SIZE + EMB_MSG_HEADER_LEN_SIZE;

//typedef struct {
//    emb_msg_header_t  header;
//    int32_t           topic_len;
//    int32_t           data_len;
//    int32_t           client_id_len;
//    char              topic[EMB_MSG_TOPIC_MAX];
//    char              data[EMB_MSG_DATA_MAX];
//    emb_id_t          client_id;
//} emb_msg_PUBLISH_t;
const EMB_MSG_PUBLISH_TOPIC_LEN_OFFSET =
    EMB_MSG_HEADER_LEN_OFFSET + EMB_MSG_HEADER_LEN_SIZE;
const EMB_MSG_PUBLISH_TOPIC_LEN_SIZE = MSG_UINT32_SIZE;
const EMB_MSG_PUBLISH_DATA_LEN_OFFSET =
    EMB_MSG_PUBLISH_TOPIC_LEN_OFFSET + EMB_MSG_PUBLISH_TOPIC_LEN_SIZE;
const EMB_MSG_PUBLISH_DATA_LEN_SIZE = MSG_UINT32_SIZE;
const EMB_MSG_PUBLISH_CLIENT_ID_LEN_OFFSET =
    EMB_MSG_PUBLISH_DATA_LEN_OFFSET + EMB_MSG_PUBLISH_DATA_LEN_SIZE;
const EMB_MSG_PUBLISH_CLIENT_ID_LEN_SIZE = MSG_UINT32_SIZE;
const EMB_MSG_PUBLISH_TOPIC_OFFSET =
    EMB_MSG_PUBLISH_CLIENT_ID_LEN_OFFSET + EMB_MSG_PUBLISH_CLIENT_ID_LEN_SIZE;
const EMB_MSG_PUBLISH_TOPIC_SIZE = EMB_MSG_TOPIC_MAX;
const EMB_MSG_PUBLISH_DATA_OFFSET =
    EMB_MSG_PUBLISH_TOPIC_OFFSET + EMB_MSG_PUBLISH_TOPIC_SIZE;
const EMB_MSG_PUBLISH_DATA_SIZE = EMB_MSG_DATA_MAX;
const EMB_MSG_PUBLISH_CLIENT_ID_OFFSET =
    EMB_MSG_PUBLISH_DATA_OFFSET + EMB_MSG_PUBLISH_DATA_SIZE;
const EMB_MSG_PUBLISH_CLIENT_ID_SIZE = MSG_UINT32_SIZE;
const EMB_MSG_PUBLISH_SIZE = EMB_MSG_PUBLISH_TOPIC_LEN_SIZE +
    EMB_MSG_PUBLISH_DATA_LEN_SIZE +
    EMB_MSG_PUBLISH_CLIENT_ID_LEN_SIZE +
    EMB_MSG_PUBLISH_TOPIC_SIZE +
    EMB_MSG_PUBLISH_DATA_SIZE +
    EMB_MSG_PUBLISH_CLIENT_ID_SIZE;

//typedef struct {
//    emb_msg_header_t  header;
//    int32_t           topic_len;
//    char              topic[EMB_MSG_TOPIC_MAX];
//} emb_msg_SUBSCRIBE_t;
const EMB_MSG_SUBSCRIBE_TOPIC_LEN_OFFSET =
    EMB_MSG_HEADER_LEN_OFFSET + EMB_MSG_HEADER_LEN_SIZE;
const EMB_MSG_SUBSCRIBE_TOPIC_LEN_SIZE = MSG_UINT32_SIZE;
const EMB_MSG_SUBSCRIBE_TOPIC_OFFSET =
    EMB_MSG_SUBSCRIBE_TOPIC_LEN_OFFSET + EMB_MSG_SUBSCRIBE_TOPIC_LEN_SIZE;
const EMB_MSG_SUBSCRIBE_TOPIC_SIZE = EMB_MSG_TOPIC_MAX;
const EMB_MSG_SUBSCRIBE_SIZE =
    EMB_MSG_SUBSCRIBE_TOPIC_LEN_SIZE + EMB_MSG_SUBSCRIBE_TOPIC_SIZE;

//typedef struct {
//    emb_msg_header_t  header;
//    int32_t           topic_len;
//    int32_t           client_id_len;
//    char              topic[EMB_MSG_TOPIC_MAX];
//    emb_id_t          client_id;
//} emb_msg_SUBACK_t;
const EMB_MSG_SUBACK_TOPIC_LEN_OFFSET =
    EMB_MSG_HEADER_LEN_OFFSET + EMB_MSG_HEADER_LEN_SIZE;
const EMB_MSG_SUBACK_TOPIC_LEN_SIZE = MSG_UINT32_SIZE;
const EMB_MSG_SUBACK_CLIENT_ID_LEN_OFFSET =
    EMB_MSG_SUBACK_TOPIC_LEN_OFFSET + EMB_MSG_SUBACK_TOPIC_LEN_SIZE;
const EMB_MSG_SUBACK_CLIENT_ID_LEN_SIZE = MSG_UINT32_SIZE;
const EMB_MSG_SUBACK_TOPIC_OFFSET =
    EMB_MSG_SUBACK_CLIENT_ID_LEN_OFFSET + EMB_MSG_SUBACK_CLIENT_ID_LEN_SIZE;
const EMB_MSG_SUBACK_TOPIC_SIZE = EMB_MSG_TOPIC_MAX;
const EMB_MSG_SUBACK_CLIENT_ID_OFFSET =
    EMB_MSG_SUBACK_TOPIC_OFFSET + EMB_MSG_SUBACK_TOPIC_SIZE;
const EMB_MSG_SUBACK_CLIENT_ID_SIZE = MSG_UINT32_SIZE;
const EMB_MSG_SUBACK_SIZE = EMB_MSG_SUBACK_TOPIC_LEN_SIZE +
    EMB_MSG_SUBACK_CLIENT_ID_LEN_SIZE +
    EMB_MSG_SUBACK_TOPIC_SIZE +
    EMB_MSG_SUBACK_CLIENT_ID_SIZE;

//typedef struct {
//    emb_msg_header_t  header;
//    int32_t           client_id_len;
//    emb_id_t          client_id;
//} emb_msg_UNSUBSCRIBE_t;
const EMB_MSG_UNSUBSCRIBE_CLIENT_ID_LEN_OFFSET =
    EMB_MSG_HEADER_LEN_OFFSET + EMB_MSG_HEADER_LEN_SIZE;
const EMB_MSG_UNSUBSCRIBE_CLIENT_ID_LEN_SIZE = MSG_UINT32_SIZE;
const EMB_MSG_UNSUBSCRIBE_CLIENT_ID_OFFSET =
    EMB_MSG_UNSUBSCRIBE_CLIENT_ID_LEN_OFFSET +
        EMB_MSG_UNSUBSCRIBE_CLIENT_ID_LEN_SIZE;
const EMB_MSG_UNSUBSCRIBE_CLIENT_ID_SIZE = MSG_UINT32_SIZE;
const EMB_MSG_UNSUBSCRIBE_SIZE =
    EMB_MSG_UNSUBSCRIBE_CLIENT_ID_LEN_SIZE + EMB_MSG_UNSUBSCRIBE_CLIENT_ID_SIZE;

//typedef struct {
//    emb_msg_header_t  header;
//    int32_t           client_id_len;
//    emb_id_t          client_id;
//} emb_msg_UNSUBACK_t;
const EMB_MSG_UNSUBACK_CLIENT_ID_LEN_OFFSET =
    EMB_MSG_HEADER_LEN_OFFSET + EMB_MSG_HEADER_LEN_SIZE;
const EMB_MSG_UNSUBACK_CLIENT_ID_LEN_SIZE = MSG_UINT32_SIZE;
const EMB_MSG_UNSUBACK_CLIENT_ID_OFFSET =
    EMB_MSG_UNSUBACK_CLIENT_ID_LEN_OFFSET + EMB_MSG_UNSUBACK_CLIENT_ID_LEN_SIZE;
const EMB_MSG_UNSUBACK_CLIENT_ID_SIZE = MSG_UINT32_SIZE;
const EMB_MSG_UNSUBACK_SIZE =
    EMB_MSG_UNSUBACK_CLIENT_ID_LEN_SIZE + EMB_MSG_UNSUBACK_CLIENT_ID_SIZE;

class SubscriberItem {
  final String topic;
  final Function handler;
  final int clientId;
  SubscriberItem(this.topic, this.handler, this.clientId);
}

class EmbClient {
  Socket? _socket;
  List<SubscriberItem>? _subscriberList;
  Completer<int>? _subCompleter;
  Completer? _unsubCompleter;

  EmbClient() {
    _subscriberList = [];
  }

  Future<void> connect(String brokerId) async {
    final host = InternetAddress(brokerId, type: InternetAddressType.unix);
    final socket = await Socket.connect(host, 0);
    print('Connected to: ${socket.remoteAddress.address}:${socket.remotePort}');
    _socket = socket;

    return;
  }

  Future<int> subscribe(String topic, Function handler) async {
    int clientId = await subscribeSync(topic);
    _subscriberList?.add(SubscriberItem(topic, handler, clientId));
    print("subscribe : clientId = $clientId ");

    print('_subscriberList.length = ${_subscriberList?.length}');

    return clientId;
  }

  Future<int> subscribeSync(String topic) async {
    _subCompleter = Completer<int>();

    _sendSubscribe(topic);

    return _subCompleter!.future;
  }

  Future<void> unsubscribe(int clientId) async {
    await unsubscribeSync(clientId);

    for (int i = 0; i < _subscriberList!.length; i++) {
      if (_subscriberList![i].clientId == clientId) {
        _subscriberList!.removeAt(i);
        break;
      }
    }

    print('_subscriberList.length = ${_subscriberList?.length}');

    return;
  }

  Future<void> unsubscribeSync(int clientId) async {
    _unsubCompleter = Completer<void>();

    _sendUnsubscribe(clientId);

    return _unsubCompleter!.future;
  }

  Future<void> publish(String topic, String data) async {
    _sendPublish(topic, data);
    return;
  }

  Future<void> run() async {
    // listen for responses from the server
    _socket!.listen(
      // handle data from the server
      (Uint8List data) {
        var offset = 0;

        while (data.length > offset) {
          var head = data.sublist(offset, offset + SS_MSG_SIZE);
          var bytedata = ByteData.view(head.buffer);
          var headSign =
              bytedata.getUint32(SS_MSG_HEAD_SIGN_OFFSET, Endian.little);
          var bodyLen =
              bytedata.getUint32(SS_MSG_BODY_LEN_OFFSET, Endian.little);

          if (headSign != SS_MSG_HEAD_SIGN) {
            print('bad head sign: ${headSign.toRadixString(16)}');
            return;
          }

          var body = data.sublist(offset, offset + SS_MSG_SIZE + bodyLen);
          bytedata = ByteData.view(body.buffer);

          var type =
              bytedata.getUint32(EMB_MSG_HEADER_TYPE_OFFSET, Endian.little);
          var len =
              bytedata.getUint32(EMB_MSG_HEADER_LEN_OFFSET, Endian.little);

          //print('recv : ${headSign.toRadixString(16)},$bodyLen,$type,$len');
          switch (type) {
            case EMB_MSG_TYPE_PUBLISH:
              _recvPublish(body);
              break;
            case EMB_MSG_TYPE_SUBACK:
              _recvSuback(body);
              break;
            case EMB_MSG_TYPE_UNSUBACK:
              _recvUnuback(body);
              break;
            default:
              break;
          }
          offset += SS_MSG_SIZE;
          offset += bodyLen;
        }
      },

      // handle errors
      onError: (error) {
        print(error);
        _socket!.destroy();
      },

      // handle server ending connection
      onDone: () {
        print('Server left.');
        _socket!.destroy();
      },
    );

    return;
  }

  Future<void> _sendSubscribe(String topic) async {
    var message =
        Uint8List(SS_MSG_SIZE + EMB_MSG_HEADER_SIZE + EMB_MSG_SUBSCRIBE_SIZE);
    var bytedata = ByteData.view(message.buffer);

    bytedata.setUint32(
        SS_MSG_HEAD_SIGN_OFFSET, SS_MSG_HEAD_SIGN, Endian.little);
    bytedata.setUint32(SS_MSG_BODY_LEN_OFFSET,
        EMB_MSG_HEADER_SIZE + EMB_MSG_SUBSCRIBE_SIZE, Endian.little);
    bytedata.setUint32(
        EMB_MSG_HEADER_TYPE_OFFSET, EMB_MSG_TYPE_SUBSCRIBE, Endian.little);
    bytedata.setUint32(EMB_MSG_HEADER_LEN_OFFSET,
        EMB_MSG_HEADER_SIZE + EMB_MSG_SUBSCRIBE_SIZE, Endian.little);

    bytedata.setUint32(EMB_MSG_SUBSCRIBE_TOPIC_LEN_OFFSET,
        EMB_MSG_PUBLISH_TOPIC_SIZE, Endian.little);

    final List<int> codeUnits = topic.codeUnits;
    final Uint8List unit8List = Uint8List.fromList(codeUnits);

    for (var i = 0; i < unit8List.length; i++) {
      bytedata.setUint8(EMB_MSG_SUBSCRIBE_TOPIC_OFFSET + i, unit8List[i]);
    }

    _socket!.add(message);

    return;
  }

  Future<void> _sendPublish(String topic, String data) async {
    var message =
        Uint8List(SS_MSG_SIZE + EMB_MSG_HEADER_SIZE + EMB_MSG_PUBLISH_SIZE);
    var bytedata = ByteData.view(message.buffer);

    bytedata.setUint32(
        SS_MSG_HEAD_SIGN_OFFSET, SS_MSG_HEAD_SIGN, Endian.little);
    bytedata.setUint32(SS_MSG_BODY_LEN_OFFSET,
        EMB_MSG_HEADER_SIZE + EMB_MSG_PUBLISH_SIZE, Endian.little);
    bytedata.setUint32(
        EMB_MSG_HEADER_TYPE_OFFSET, EMB_MSG_TYPE_PUBLISH, Endian.little);
    bytedata.setUint32(EMB_MSG_HEADER_LEN_OFFSET,
        EMB_MSG_HEADER_SIZE + EMB_MSG_PUBLISH_SIZE, Endian.little);

    bytedata.setUint32(EMB_MSG_PUBLISH_TOPIC_LEN_OFFSET,
        EMB_MSG_PUBLISH_TOPIC_SIZE, Endian.little);
    bytedata.setUint32(EMB_MSG_PUBLISH_DATA_LEN_OFFSET,
        EMB_MSG_PUBLISH_DATA_SIZE, Endian.little);
    bytedata.setUint32(EMB_MSG_PUBLISH_CLIENT_ID_LEN_OFFSET,
        EMB_MSG_PUBLISH_CLIENT_ID_SIZE, Endian.little);

    final List<int> topicCodeUnits = topic.codeUnits;
    final Uint8List topicUint8List = Uint8List.fromList(topicCodeUnits);

    for (var i = 0; i < topicUint8List.length; i++) {
      bytedata.setUint8(EMB_MSG_PUBLISH_TOPIC_OFFSET + i, topicUint8List[i]);
    }

    final List<int> dataCodeUnits = data.codeUnits;
    final Uint8List dataUint8List = Uint8List.fromList(dataCodeUnits);

    for (var i = 0; i < dataUint8List.length; i++) {
      bytedata.setUint8(EMB_MSG_PUBLISH_DATA_OFFSET + i, dataUint8List[i]);
    }

    bytedata.setUint32(
        EMB_MSG_PUBLISH_CLIENT_ID_OFFSET, EMB_ID_BROADCAST, Endian.little);

    _socket!.add(message);

    return;
  }

  Future<void> _sendUnsubscribe(int clientId) async {
    var message =
        Uint8List(SS_MSG_SIZE + EMB_MSG_HEADER_SIZE + EMB_MSG_UNSUBSCRIBE_SIZE);
    var bytedata = ByteData.view(message.buffer);

    bytedata.setUint32(
        SS_MSG_HEAD_SIGN_OFFSET, SS_MSG_HEAD_SIGN, Endian.little);
    bytedata.setUint32(SS_MSG_BODY_LEN_OFFSET,
        EMB_MSG_HEADER_SIZE + EMB_MSG_UNSUBSCRIBE_SIZE, Endian.little);
    bytedata.setUint32(
        EMB_MSG_HEADER_TYPE_OFFSET, EMB_MSG_TYPE_UNSUBSCRIBE, Endian.little);
    bytedata.setUint32(EMB_MSG_HEADER_LEN_OFFSET,
        EMB_MSG_HEADER_SIZE + EMB_MSG_UNSUBSCRIBE_SIZE, Endian.little);

    bytedata.setUint32(EMB_MSG_UNSUBSCRIBE_CLIENT_ID_LEN_OFFSET,
        EMB_MSG_UNSUBSCRIBE_CLIENT_ID_SIZE, Endian.little);
    bytedata.setUint32(
        EMB_MSG_UNSUBSCRIBE_CLIENT_ID_OFFSET, clientId, Endian.little);

    _socket!.add(message);

    return;
  }

  void _recvSuback(Uint8List msg) {
    var bytedata = ByteData.view(msg.buffer);

    var topic_len =
        bytedata.getUint32(EMB_MSG_SUBACK_TOPIC_LEN_OFFSET, Endian.little);
    var client_id_len =
        bytedata.getUint32(EMB_MSG_SUBACK_CLIENT_ID_LEN_OFFSET, Endian.little);
    var client_id =
        bytedata.getUint32(EMB_MSG_SUBACK_CLIENT_ID_OFFSET, Endian.little);

    String topic = String.fromCharCodes(
        msg, EMB_MSG_SUBACK_TOPIC_OFFSET, EMB_MSG_SUBACK_TOPIC_SIZE);

    print('_recvSuback : $topic, $client_id');

    _subCompleter!.complete(client_id);
  }

  void _recvUnuback(Uint8List msg) {
    var bytedata = ByteData.view(msg.buffer);

    var client_id_len = bytedata.getUint32(
        EMB_MSG_UNSUBACK_CLIENT_ID_LEN_OFFSET, Endian.little);

    var client_id =
        bytedata.getUint32(EMB_MSG_UNSUBACK_CLIENT_ID_OFFSET, Endian.little);

    print('_recvUnuback : $client_id');

    _unsubCompleter!.complete();
  }

  void _recvPublish(Uint8List msg) {
    var bytedata = ByteData.view(msg.buffer);

    var topic_len =
        bytedata.getUint32(EMB_MSG_PUBLISH_TOPIC_LEN_OFFSET, Endian.little);
    var data_len =
        bytedata.getUint32(EMB_MSG_PUBLISH_DATA_LEN_OFFSET, Endian.little);
    var client_id_len =
        bytedata.getUint32(EMB_MSG_PUBLISH_CLIENT_ID_LEN_OFFSET, Endian.little);

    var client_id =
        bytedata.getUint32(EMB_MSG_PUBLISH_CLIENT_ID_OFFSET, Endian.little);

    String topic = String.fromCharCodes(
        msg, EMB_MSG_PUBLISH_TOPIC_OFFSET, EMB_MSG_PUBLISH_TOPIC_SIZE);

    String data = String.fromCharCodes(
        msg, EMB_MSG_PUBLISH_DATA_OFFSET, EMB_MSG_PUBLISH_DATA_SIZE);

    print('_recvPublish : $topic, $data, $client_id');

    _subscriberList?.forEach((it) {
      if (it.clientId == client_id) {
        it.handler(topic, data);
      }
    });
  }
}
