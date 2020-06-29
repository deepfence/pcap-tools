.. zeek:type:: TestEnum1

   :Type: :zeek:type:`enum`

      .. zeek:enum:: ONE TestEnum1

         like this

      .. zeek:enum:: TWO TestEnum1

         or like this

      .. zeek:enum:: THREE TestEnum1

         multiple
         comments
         and even
         more comments

      .. zeek:enum:: FOUR TestEnum1

         adding another
         value

      .. zeek:enum:: FIVE TestEnum1

         adding another
         value

   There's tons of ways an enum can look...

.. zeek:type:: TestEnum2

   :Type: :zeek:type:`enum`

      .. zeek:enum:: A TestEnum2

         like this

      .. zeek:enum:: B TestEnum2

         or like this

      .. zeek:enum:: C TestEnum2

         multiple
         comments
         and even
         more comments

   The final comma is optional

.. zeek:id:: TestEnumVal

   :Type: :zeek:type:`TestEnum1`
   :Attributes: :zeek:attr:`&redef`
   :Default: ``ONE``

   this should reference the TestEnum1 type and not a generic "enum" type

