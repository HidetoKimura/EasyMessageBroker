import 'package:emb_client/emb_client.dart' as Emb;

void main(List<String> arguments) async {
  var emb = Emb.EmbClient();
  await emb.connect('/tmp/test');
  await emb.run();
  var id1 = await emb.subscribe('/signal/power', (topic, data) {
    print('handler1: $topic, $data');
  });
  await emb.publish('/signal/power', 'power_on');

  var id2 = await emb.subscribe('/signal/power', (topic, data) {
    print('handler2: $topic, $data');
  });

  await emb.publish('/signal/power', 'power_off');

  await emb.unsubscribe(id1);

  await emb.unsubscribe(id2);
}
