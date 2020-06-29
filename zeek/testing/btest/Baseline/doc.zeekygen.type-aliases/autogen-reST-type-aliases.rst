.. zeek:type:: ZeekygenTest::TypeAlias

   :Type: :zeek:type:`bool`

   This is just an alias for a builtin type ``bool``.

.. zeek:type:: ZeekygenTest::NotTypeAlias

   :Type: :zeek:type:`bool`

   This type should get its own comments, not associated w/ TypeAlias.

.. zeek:type:: ZeekygenTest::OtherTypeAlias

   :Type: :zeek:type:`bool`

   This cross references ``bool`` in the description of its type
   instead of ``TypeAlias`` just because it seems more useful --
   one doesn't have to click through the full type alias chain to
   find out what the actual type is...

.. zeek:id:: ZeekygenTest::a

   :Type: :zeek:type:`ZeekygenTest::TypeAlias`

   But this should reference a type of ``TypeAlias``.

.. zeek:id:: ZeekygenTest::b

   :Type: :zeek:type:`ZeekygenTest::OtherTypeAlias`

   And this should reference a type of ``OtherTypeAlias``.

.. zeek:type:: ZeekygenTest::MyRecord

   :Type: :zeek:type:`record`

      f1: :zeek:type:`ZeekygenTest::TypeAlias`

      f2: :zeek:type:`ZeekygenTest::OtherTypeAlias`

      f3: :zeek:type:`bool`


