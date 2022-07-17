import 'package:emb_client/emb_client.dart' as emb_client;

void main(List<String> arguments) {
  emb_client.start('/tmp/test', (topic, data) {
    print('handler: $topic, $data');
  });
}
