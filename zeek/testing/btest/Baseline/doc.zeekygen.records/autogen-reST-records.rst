.. zeek:type:: TestRecord1

   :Type: :zeek:type:`record`

      field1: :zeek:type:`bool`

      field2: :zeek:type:`count`


.. zeek:type:: TestRecord2

   :Type: :zeek:type:`record`

      A: :zeek:type:`count`
         document ``A``

      B: :zeek:type:`bool`
         document ``B``

      C: :zeek:type:`TestRecord1`
         and now ``C``
         is a declared type

      D: :zeek:type:`set` [:zeek:type:`count`, :zeek:type:`bool`]
         sets/tables should show the index types

   Here's the ways records and record fields can be documented.

