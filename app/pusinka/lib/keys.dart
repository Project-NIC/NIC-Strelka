/// What the phone keeps of each Povodok in its own keystore (APP.md, What it must do, 1;
/// COMMANDS.md, The lock): the password, entered once, and Povodok's own key, pinned at the first
/// joining, both by Povodok's address. No keystore — a test — and nothing is kept.
library;

import 'dart:convert';

import 'package:flutter_secure_storage/flutter_secure_storage.dart';

abstract final class Keys {
  static const _store = FlutterSecureStorage();

  static Future<String?> _read(String k) async {
    try {
      return await _store.read(key: k);
    } on Object {
      return null;
    }
  }

  static Future<void> _write(String k, String v) async {
    try {
      await _store.write(key: k, value: v);
    } on Object {
      // no keystore: kept for this run alone
    }
  }

  static Future<String?> password(String host) => _read('pw:$host');

  static Future<void> setPassword(String host, String password) => _write('pw:$host', password);

  static Future<List<int>?> pinned(String host) async {
    final v = await _read('key:$host');
    return v == null ? null : base64Decode(v);
  }

  static Future<void> pin(String host, List<int> key) => _write('key:$host', base64Encode(key));

  /// The set drawn last, kept to be given to each Povodok in turn, its key a secret as the
  /// password is (APP.md, What it must do, 3).
  static Future<String?> newSet() => _read('set:new');

  static Future<void> setNewSet(String json) => _write('set:new', json);
}
