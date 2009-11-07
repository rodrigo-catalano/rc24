using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;
using rc24;

namespace Serial
{
    public class CC_EnumTypeConverter : ByteConverter
    {
        public CC_EnumTypeConverter()
        {

        }
        public override bool GetStandardValuesSupported(ITypeDescriptorContext context)
        {
            return true;
        }
        public override bool GetStandardValuesExclusive(ITypeDescriptorContext context)
        {
            return true;
        }
        public override StandardValuesCollection GetStandardValues(ITypeDescriptorContext context)
        {
            if (context == null) return null;
            CC_paramBinder pb = (CC_paramBinder)context.Instance;
            ccParameter param= pb.Node.properties[context.PropertyDescriptor.Name];

            if (param != null)
            {
                return new StandardValuesCollection(param.GetEnumList());
            }
            else return null;
        }
        public override bool CanConvertFrom(System.ComponentModel.ITypeDescriptorContext context, System.Type sourceType)
        {
            if (sourceType == typeof(string))
                return true;
            else if (sourceType == typeof(byte))
                return true;
            else
                return base.CanConvertFrom(context, sourceType);
        }
        public override object ConvertFrom(System.ComponentModel.ITypeDescriptorContext context, System.Globalization.CultureInfo culture, object value)
        {
            if (value is byte)
            {
                return GetStandardValues(context)[(int)value]; //used
            }

            if (value is string)
            {
                StandardValuesCollection values = GetStandardValues(context);

                byte i = 0;
                foreach (string s in values)
                {
                    if (s == (string)value) return i;
                    i++;
                }

            }
            return base.ConvertFrom(context, culture, value);
        }

        public override bool CanConvertTo(ITypeDescriptorContext context, Type t)
        {
            if (t == typeof(string))
                return true;
            else if (t == typeof(byte))
                return true;
            else
                return base.CanConvertTo(context, t);
        }
        public override object ConvertTo(ITypeDescriptorContext context, System.Globalization.CultureInfo culture, object value, Type destinationType)
        {
            StandardValuesCollection values = GetStandardValues(context);

            if (destinationType == typeof(string) && value != null && value is byte)
            {
                if (values.Count > (byte)value)
                {
                    return GetStandardValues(context)[(byte)value]; //used
                }
            }
            if (destinationType == typeof(byte) && value != null) //not used
            {
                byte i = 0;
                foreach (string s in values)
                {
                    if (s == (string)value) return i;
                    i++;
                }

            }
            return base.ConvertTo(context, culture, value, destinationType);
        }

    }
}
