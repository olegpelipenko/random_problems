__author__ = 'pelipenko'
import re


class BaseString(object):
    def __init__(self, text=''):
        self.text = text

    text = ''

    def to_string(self):
        return self.text


class EmptyString(BaseString):
    def __repr__(self):
        return "EmptyString: ''"


class NotValidString(BaseString):
    def __repr__(self):
        return "NotValidString: '{0}'".format(self.text)


class Comment(BaseString):
    def __repr__(self):
        return "Comment:'{0}'".format(self.text)


class KeyValue(object):
    def __init__(self, key='', value=''):
        self.key = key
        self.value = value
    key = ''
    value = ''

    def to_string(self):
        return '{0} {1}'.format(self.key, self.value)

    def __repr__(self):
        return "KeyValue: '{0}' '{1}'".format(self.key, self.value)

    def to_key_value(self):
        return self

    def __eq__(self, other):
        return self.key == other.key and self.value == other.value


class KeyValueWithComment(KeyValue):
    comment = ''

    def __init__(self, key='', value='', comment=''):
        KeyValue.__init__(self, key, value)
        self.comment = comment

    def to_string(self):
        return '{0} {1}'.format(KeyValue.to_string(self), self.comment)

    def __repr__(self):
        return "KeyValueWithComment: '{0}' '{1}'".format(self.key, self.value)

    def to_key_value(self):
        return KeyValue(self.key, self.value)


class FileSerializer:
    def __init__(self, filename):
        self._filename = filename
        self._file = None
        self._objects = list()
        self._current = 0
        self._is_reversed = False
        self._keyRE = r'^\s*([a-zA-Z]+[a-zA-Z0-9-_]*)'

    def open_file(self, mode='r', reversed_order=False):
        self._file = open(self._filename, mode)
        self._is_reversed = reversed_order

        if self._is_reversed:
            for line in reversed(self._file.readlines()):
                self._objects.append(self._parse_line(line))
        else:
            for line in self._file.readlines():
                self._objects.append(self._parse_line(line))
        self._current = 0

    def close_file(self):
        if self._file is not None and not self._file.closed:
            self._file.close()
        self._objects = list()

    def read_next_object(self):
        if len(self._objects) > self._current:
            some_object = self._objects[self._current]
            self._current += 1
            return some_object
        else:
            return None

    def remove_current(self):
        del self._objects[self._current - 1]
        if len(self._objects) > 0:
            if self._current < len(self._objects):
                self._current -= 1
            else:
                self._current = 0

    def add_key_value(self, obj):
        if self._is_reversed:
            self._objects.insert(0, obj)
        else:
            self._objects.append(obj)

    def flush(self):
        # clear file
        self._file.seek(0)
        self._file.truncate()

        # write new content
        lines = list()
        if self._is_reversed:
            for obj in reversed(self._objects):
                lines.append(obj.to_string() + '\n')
        else:
            for obj in self._objects:
                lines.append(obj.to_string() + '\n')

        self._file.writelines(lines)
        return

    def is_key_name_valid(self, key):
        return re.match(self._keyRE, key) is not None

    def _parse_line(self, line=''):
        if line.isspace():
            return EmptyString()

        stripped_line = line.strip()
        found = re.findall(self._keyRE + r'\s+((?:\b[\S]+\s*)*)(#.*)?$', stripped_line)
        if len(found) == 0:
            if stripped_line[0] == '#':
                return Comment(stripped_line)
            else:
                return NotValidString(stripped_line)
        else:
            if len(found[0][2]) == 0:
                return KeyValue(found[0][0], found[0][1].strip())
            else:
                return KeyValueWithComment(found[0][0], found[0][1].strip(), found[0][2])


class SettingsParser:
    def __init__(self, filename=''):
        self._serializer = FileSerializer(filename)

    # returns list of KeyValues in file
    def read_all_keys(self):
        self._serializer.open_file('r', False)
        key_value_objects = list()
        current = self._serializer.read_next_object()

        while current is not None:
            if self._is_key_value(current):
                key_value_objects.append(current.to_key_value())
            current = self._serializer.read_next_object()

        self._serializer.close_file()
        return key_value_objects

    # returns KeyValue by key or None
    # reads from end of file
    def get(self, key=''):
        self._serializer.open_file('r', True)
        current = self._serializer.read_next_object()
        found = None

        while current is not None:
            if self._is_key_value(current):
                if current.key == key:
                    found = current
                    break
            current = self._serializer.read_next_object()

        self._serializer.close_file()
        return found

    # replace KeyValue in file
    # reads from end of file
    def set(self, key_value):
        if not self._serializer.is_key_name_valid(key_value.key):
            return False

        self._serializer.open_file('r+', True)
        current = self._serializer.read_next_object()
        is_found = False

        while current is not None:
            if self._is_key_value(current):
                if current.key == key_value.key:
                    is_found = True
                    current.value = key_value.value
                    break

            current = self._serializer.read_next_object()

        if not is_found:
            self._serializer.add_key_value(key_value)

        self._serializer.flush()
        self._serializer.close_file()

        return True

    # remove key with value from file
    # reads from end of file
    def remove(self, key=''):
        self._serializer.open_file('r+', True)
        current = self._serializer.read_next_object()

        while current is not None:
            if self._is_key_value(current) and current.key == key:
                self._serializer.remove_current()
                break

            current = self._serializer.read_next_object()

        self._serializer.flush()
        self._serializer.close_file()

    @staticmethod
    def _is_key_value(probably_key_value_object):
        return isinstance(probably_key_value_object, (KeyValueWithComment, KeyValue))