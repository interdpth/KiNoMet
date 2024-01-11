using System;
using System.Collections.Generic;

namespace KinometGui
{
    class Compression
    {
        public static long DecompRLE(IOStream input, IOStream output)
        {
            long position;
            position = input.Position;
            long position2;
            position2 = output.Position;
            for (int i = 0; i < 2; i++)
            {
                long num;
                num = position2 + i;
                byte b;
                b = input.Read8();
                while (true)
                {
                    ushort num3;
                    ushort num2;
                    if (b == 1)
                    {
                        num2 = input.Read8();
                        num3 = 128;
                    }
                    else
                    {
                        num2 = (ushort)((input.Read8() << 8) + input.Read8());
                        num3 = 32768;
                    }
                    if (num2 == 0)
                    {
                        break;
                    }
                    if ((num2 & num3) != 0)
                    {
                        num2 = (ushort)((int)num2 % (int)num3);
                        byte val;
                        val = input.Read8();
                        while (num2 > 0)
                        {
                            output.Seek((int)num);
                            output.Write8(val);
                            num += 2;
                            num2 = (ushort)(num2 - 1);
                        }
                    }
                    else
                    {
                        while (num2 > 0)
                        {
                            output.Seek((int)num);
                            output.Write8(input.Read8());
                            num += 2;
                            num2 = (ushort)(num2 - 1);
                        }
                    }
                }
            }
            return input.Position - position;
        }

        public static long CompRLE(IOStream input, int length, IOStream output)
        {
            long position;
            position = output.Position;
            byte[] data;
            data = input.Data;
            for (int i = 0; i < 2; i++)
            {
                List<byte> list;
                list = new List<byte>();
                List<int> list2;
                list2 = new List<int>();
                byte b;
                b = data[i];
                list.Add(b);
                int num;
                num = 1;
                for (int j = i + 2; j < length; j += 2)
                {
                    byte b2;
                    b2 = data[j];
                    if (b2 == b)
                    {
                        num++;
                        continue;
                    }
                    list.Add(b2);
                    list2.Add(num);
                    b = b2;
                    num = 1;
                }
                list2.Add(num);
                byte[,] array;
                array = new byte[2, length];
                int[] array2;
                array2 = new int[2];
                int[] array3;
                array3 = array2;
                for (int k = 0; k < 2; k++)
                {
                    int num2;
                    num2 = 3 + k;
                    int num3;
                    num3 = 128 << 8 * k;
                    int num4;
                    num4 = num3 - 1;
                    List<byte> list3;
                    list3 = new List<byte>();
                    array[k, array3[k]++] = (byte)(k + 1);
                    for (int l = 0; l < list.Count; l++)
                    {
                        num = list2[l];
                        if (num >= num2)
                        {
                            if (list3.Count > 0)
                            {
                                if (k == 0)
                                {
                                    array[k, array3[k]++] = (byte)list3.Count;
                                }
                                else
                                {
                                    array[k, array3[k]++] = (byte)(list3.Count >> 8);
                                    array[k, array3[k]++] = (byte)list3.Count;
                                }
                                foreach (byte item in list3)
                                {
                                    array[k, array3[k]++] = item;
                                }
                                list3.Clear();
                            }
                            while (num > 0)
                            {
                                int num5;
                                num5 = num3 + Math.Min(num, num4);
                                if (k == 0)
                                {
                                    array[k, array3[k]++] = (byte)num5;
                                }
                                else
                                {
                                    array[k, array3[k]++] = (byte)(num5 >> 8);
                                    array[k, array3[k]++] = (byte)num5;
                                }
                                array[k, array3[k]++] = list[l];
                                num -= num4;
                            }
                            continue;
                        }
                        if (list3.Count + num > num4)
                        {
                            if (k == 0)
                            {
                                array[k, array3[k]++] = (byte)list3.Count;
                            }
                            else
                            {
                                array[k, array3[k]++] = (byte)(list3.Count >> 8);
                                array[k, array3[k]++] = (byte)list3.Count;
                            }
                            foreach (byte item2 in list3)
                            {
                                array[k, array3[k]++] = item2;
                            }
                            list3.Clear();
                        }
                        for (int m = 0; m < num; m++)
                        {
                            list3.Add(list[l]);
                        }
                    }
                    if (list3.Count > 0)
                    {
                        if (k == 0)
                        {
                            array[k, array3[k]++] = (byte)list3.Count;
                        }
                        else
                        {
                            array[k, array3[k]++] = (byte)(list3.Count >> 8);
                            array[k, array3[k]++] = (byte)list3.Count;
                        }
                        foreach (byte item3 in list3)
                        {
                            array[k, array3[k]++] = item3;
                        }
                        list3.Clear();
                    }
                    array[k, array3[k]++] = 0;
                    if (k == 1)
                    {
                        array[k, array3[k]++] = 0;
                    }
                }
                int num6;
                num6 = ((array3[0] > array3[1]) ? 1 : 0);
                int num7;
                num7 = array3[num6];
                for (int n = 0; n < num7; n++)
                {
                    output.Write8(array[num6, n]);
                }
            }
            return output.Position - position;
        }

        public static long DecompLZ77(IOStream input, IOStream output)
        {
            long position;
            position = input.Position;
            int num;
            num = input.Read32() >> 8;
            while (num > 0)
            {
                byte b;
                b = input.Read8();
                for (int i = 0; i < 8; i++)
                {
                    if ((b & 0x80) == 0)
                    {
                        output.Write8(input.Read8());
                        num--;
                    }
                    else
                    {
                        byte b2;
                        b2 = input.Read8();
                        byte b3;
                        b3 = input.Read8();
                        int num2;
                        num2 = (b2 >> 4) + 3;
                        int window;
                        window = ((b2 & 0xF) << 8) + b3 + 1;
                        num -= num2;
                        output.OverlappingCopy(num2, window);
                    }
                    if (num <= 0)
                    {
                        return input.Position - position;
                    }
                    b = (byte)(b << 1);
                }
            }
            return input.Position - position;
        }

        public static long CompLZ77(IOStream input, int length, IOStream output)
        {
            byte[] data;
            data = input.Data;
            Dictionary<int, List<int>> dictionary;
            dictionary = new Dictionary<int, List<int>>();
            for (int i = 0; i < input.Length - 2; i++)
            {
                int key;
                key = (data[i] | (data[i + 1] << 8) | (data[i + 2] << 16));
                List<int> value;
                if (dictionary.TryGetValue(key, out value))
                {
                    value.Add(i);
                }
                else
                {
                    dictionary.Add(key, new List<int>
                    {
                        i
                    });
                }
            }
            int num;
            num = 18;
            int num2;
            num2 = 4096;
            int num3;
            num3 = 0;
            long position;
            position = output.Position;
            output.Write8(16);
            output.Write8((byte)length);
            output.Write8((byte)(length >> 8));
            output.Write8((byte)(length >> 16));
            while (num3 < length)
            {
                long position2;
                position2 = output.Position;
                output.Write8(0);
                for (int num4 = 0; num4 < 8; num4++)
                {
                    if (num3 + 3 <= length)
                    {
                        int key2;
                        key2 = (data[num3] | (data[num3 + 1] << 8) | (data[num3 + 2] << 16));
                        List<int> value2;
                        if (dictionary.TryGetValue(key2, out value2))
                        {
                            int j;
                            j = 0;
                            while (value2[j] < num3 - num2)
                            {
                                j++;
                                if (j != value2.Count)
                                {
                                    continue;
                                }
                                goto IL_01cf;
                            }
                            int num5;
                            num5 = -1;
                            int num6;
                            num6 = -1;
                            for (; j < value2.Count; j++)
                            {
                                int num7;
                                num7 = value2[j];
                                if (num7 >= num3 - 1)
                                {
                                    break;
                                }
                                int k;
                                for (k = 3; num3 + k < length && data[num7 + k] == data[num3 + k] && k < num; k++)
                                {
                                }
                                if (k > num5)
                                {
                                    num5 = k;
                                    num6 = num7;
                                }
                            }
                            if (num6 != -1)
                            {
                                int num8;
                                num8 = num3 - num6 - 1;
                                output.Write8((byte)((num5 - 3 << 4) | (num8 >> 8)));
                                output.Write8((byte)num8);
                                output.Data[position2] |= (byte)(128 >> num4);
                                num3 += num5;
                                goto IL_01de;
                            }
                        }
                    }
                    goto IL_01cf;
                IL_01cf:
                    output.Write8(data[num3++]);
                    goto IL_01de;
                IL_01de:
                    if (num3 >= length)
                    {
                        break;
                    }
                }
            }
            return output.Position - position;
        }
    }
}

