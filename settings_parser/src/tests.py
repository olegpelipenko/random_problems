__author__ = 'pelipenko'

import unittest
from tempfile import mkstemp

path_to_config_folder = '../examples/'
import imp
settings_parser = imp.load_source('SettingsParser', 'settings_parser.py')
KeyValue = settings_parser.KeyValue


class TestSettingsParser(unittest.TestCase):
    def _copy_to_temp_file(self, original_file_name):
        fp, temp_file_path = mkstemp()
        temp = open(temp_file_path, 'w')
        with open(original_file_name) as f:
            temp.writelines(f.readlines())
        temp.close()
        return temp_file_path

    def setUp(self):
        self._temp_file_path = self._copy_to_temp_file(path_to_config_folder + 'config.txt')
        self._parser = settings_parser.SettingsParser(self._temp_file_path)

    def test_read_keys(self):
        # get all keys and values
        keys = self._parser.read_all_keys()
        self.assertEqual(len(keys), 5)
        pairs = [KeyValue('key1', 'i\'m a value of key1'),
                 KeyValue('key2', 'value followed by comment'),
                 KeyValue('key3', 'value with hash# sign'),
                 KeyValue('key3', 'repeated value of key 3'),
                 KeyValue('key_with-long-strange_name', '')
        ]
        self.assertListEqual(keys, pairs)

        # simple get
        self.assertEqual(self._parser.get('key3'), KeyValue('key3', 'repeated value of key 3'))

    def test_set_key(self):
        # set valid key
        key2 = KeyValue('key2', 'some new value')
        self._parser.set(key2)
        keys = self._parser.read_all_keys()
        key2_in_file = [x for x in keys if (x.key == 'key2')]
        self.assertEqual(key2_in_file[0], key2)

        # set not valid key
        not_valid_key = KeyValue('2key', 'some value')
        self.assertFalse(self._parser.set(not_valid_key))
        key2_in_file = [x for x in keys if (x.key == '2key')]
        self.assertEqual(len(key2_in_file), 0)

    def test_remove_key(self):
        key_to_remove = 'key1'
        self.assertNotEqual(self._parser.get(key_to_remove), None)
        self._parser.remove(key_to_remove)
        self.assertEqual(self._parser.get(key_to_remove), None)

if __name__ == '__main__':
    unittest.main()