.. zeek:id:: ZeekygenExample::Zeekygen_One

   :Type: :zeek:type:`Notice::Type`

   Any number of this type of comment
   will document "Zeekygen_One".

.. zeek:id:: ZeekygenExample::Zeekygen_Two

   :Type: :zeek:type:`Notice::Type`

   Any number of this type of comment
   will document "ZEEKYGEN_TWO".

.. zeek:id:: ZeekygenExample::Zeekygen_Three

   :Type: :zeek:type:`Notice::Type`


.. zeek:id:: ZeekygenExample::Zeekygen_Four

   :Type: :zeek:type:`Notice::Type`

   Omitting comments is fine, and so is mixing ``##`` and ``##<``, but
   it's probably best to use only one style consistently.

.. zeek:id:: ZeekygenExample::LOG

   :Type: :zeek:type:`Log::ID`


.. zeek:type:: ZeekygenExample::SimpleEnum

   :Type: :zeek:type:`enum`

      .. zeek:enum:: ZeekygenExample::ONE ZeekygenExample::SimpleEnum

         Documentation for particular enum values is added like this.
         And can also span multiple lines.

      .. zeek:enum:: ZeekygenExample::TWO ZeekygenExample::SimpleEnum

         Or this style is valid to document the preceding enum value.

      .. zeek:enum:: ZeekygenExample::THREE ZeekygenExample::SimpleEnum

      .. zeek:enum:: ZeekygenExample::FOUR ZeekygenExample::SimpleEnum

         And some documentation for "FOUR".

      .. zeek:enum:: ZeekygenExample::FIVE ZeekygenExample::SimpleEnum

         Also "FIVE".

   Documentation for the "SimpleEnum" type goes here.
   It can span multiple lines.

.. zeek:id:: ZeekygenExample::ONE

   :Type: :zeek:type:`ZeekygenExample::SimpleEnum`

   Documentation for particular enum values is added like this.
   And can also span multiple lines.

.. zeek:id:: ZeekygenExample::TWO

   :Type: :zeek:type:`ZeekygenExample::SimpleEnum`

   Or this style is valid to document the preceding enum value.

.. zeek:id:: ZeekygenExample::THREE

   :Type: :zeek:type:`ZeekygenExample::SimpleEnum`


.. zeek:id:: ZeekygenExample::FOUR

   :Type: :zeek:type:`ZeekygenExample::SimpleEnum`

   And some documentation for "FOUR".

.. zeek:id:: ZeekygenExample::FIVE

   :Type: :zeek:type:`ZeekygenExample::SimpleEnum`

   Also "FIVE".

.. zeek:type:: ZeekygenExample::SimpleRecord

   :Type: :zeek:type:`record`

      field1: :zeek:type:`count`
         Counts something.

      field2: :zeek:type:`bool`
         Toggles something.

      field_ext: :zeek:type:`string` :zeek:attr:`&optional`
         Document the extending field like this.
         Or here, like this.

   General documentation for a type "SimpleRecord" goes here.
   The way fields can be documented is similar to what's already seen
   for enums.

.. zeek:type:: ZeekygenExample::ComplexRecord

   :Type: :zeek:type:`record`

      field1: :zeek:type:`count`
         Counts something.

      field2: :zeek:type:`bool`
         Toggles something.

      field3: :zeek:type:`ZeekygenExample::SimpleRecord`
         Zeekygen automatically tracks types
         and cross-references are automatically
         inserted in to generated docs.

      msg: :zeek:type:`string` :zeek:attr:`&default` = ``"blah"`` :zeek:attr:`&optional`
         Attributes are self-documenting.
   :Attributes: :zeek:attr:`&redef`

   General documentation for a type "ComplexRecord" goes here.

.. zeek:type:: ZeekygenExample::Info

   :Type: :zeek:type:`record`

      ts: :zeek:type:`time` :zeek:attr:`&log`

      uid: :zeek:type:`string` :zeek:attr:`&log`

      status: :zeek:type:`count` :zeek:attr:`&log` :zeek:attr:`&optional`

   An example record to be used with a logging stream.
   Nothing special about it.  If another script redefs this type
   to add fields, the generated documentation will show all original
   fields plus the extensions and the scripts which contributed to it
   (provided they are also @load'ed).

.. zeek:id:: ZeekygenExample::an_option

   :Type: :zeek:type:`set` [:zeek:type:`addr`, :zeek:type:`addr`, :zeek:type:`string`]
   :Attributes: :zeek:attr:`&redef`
   :Default: ``{}``

   Add documentation for "an_option" here.
   The type/attribute information is all generated automatically.

.. zeek:id:: ZeekygenExample::option_with_init

   :Type: :zeek:type:`interval`
   :Attributes: :zeek:attr:`&redef`
   :Default: ``10.0 msecs``

   Default initialization will be generated automatically.
   More docs can be added here.

.. zeek:id:: ZeekygenExample::a_var

   :Type: :zeek:type:`bool`

   Put some documentation for "a_var" here.  Any global/non-const that
   isn't a function/event/hook is classified as a "state variable"
   in the generated docs.

.. zeek:id:: ZeekygenExample::var_without_explicit_type

   :Type: :zeek:type:`string`
   :Default: ``"this works"``

   Types are inferred, that information is self-documenting.

.. zeek:id:: ZeekygenExample::summary_test

   :Type: :zeek:type:`string`

   The first sentence for a particular identifier's summary text ends here.
   And this second sentence doesn't show in the short description provided
   by the table of all identifiers declared by this script.

.. zeek:id:: ZeekygenExample::a_function

   :Type: :zeek:type:`function` (tag: :zeek:type:`string`, msg: :zeek:type:`string`) : :zeek:type:`string`

   Summarize purpose of "a_function" here.
   Give more details about "a_function" here.
   Separating the documentation of the params/return values with
   empty comments is optional, but improves readability of script.
   

   :tag: Function arguments can be described
        like this.
   

   :msg: Another param.
   

   :returns: Describe the return type here.

.. zeek:id:: ZeekygenExample::an_event

   :Type: :zeek:type:`event` (name: :zeek:type:`string`)

   Summarize "an_event" here.
   Give more details about "an_event" here.
   
   ZeekygenExample::a_function should not be confused as a parameter
   in the generated docs, but it also doesn't generate a cross-reference
   link.  Use the see role instead: :zeek:see:`ZeekygenExample::a_function`.
   

   :name: Describe the argument here.

.. zeek:id:: ZeekygenExample::function_without_proto

   :Type: :zeek:type:`function` (tag: :zeek:type:`string`) : :zeek:type:`string`


.. zeek:type:: ZeekygenExample::PrivateRecord

   :Type: :zeek:type:`record`

      field1: :zeek:type:`bool`

      field2: :zeek:type:`count`


