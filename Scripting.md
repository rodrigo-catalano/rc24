# Introduction #

It is now possible to run scripts on the receiver or pc (transmitter to follow ).  It is early days so much of below is likely to change



## Syntax ##
The script syntax is custom but similar to a subset of c,c#,java,javascript etc.

methods/functions are declared the same way eg
```
num foo(num a, num b)
{
    return(a*b)
}
```

// is used to comment out the rest of a line
There are no semicolons at the end of statements.


methods are called the normal way

c=foo(10,20)

optionally the parameter names can be used

c=foo(a: 10,b: 20)


variables are strongly typed but do not need to be declared, their type is inferred.

Probably the least familiar feature of the language is the absence of special control structures for things like looping and if/else.  Instead these are handled by allowing blocks of code to be passed to methods as parameters.

e.g.
```
int a=20;
while(a>1)
{
  print(a);
  a=a-1;
}
```

is achieved by

```
a=20
while(a>1,
  print(a)
  a=a-1
)
```

'while' is just a function defined to accept two blocks of code
This allows you to effectively create new control structures

If you prefer you can use named parameters to write
```
while(test: a>1,
  do:
    print(a)
    a=a-1
)
```

The script is entered by the containing code calling the main function
```
num main(TX tx,RX rx, pilot pilot,IMU imu)
{

}
```

Objects such as tx are the same objects you see when you browse the tree of connected devices on the pc program.  The properties of each of these objects are discovered at compile time by interrogating whatever is connected to the serial port.
This means that if you add extra exposed variables to the rx or tx bin files, they will be available in the script.  The parameter names are the same as on the pc display but with spaces removed.

To set the demand for channel 1 on the tramsmitter to be twice the channel 2 demand -

```
  a=tx.TXInputs[2]
  tx.TXInputs[1]=a*2)
```


## Operators ##
+
-
`*`
/
`%`

`==`
!=
>
>=
<
<=

the ++ and -- operators are not supported (yet)

## methods ##