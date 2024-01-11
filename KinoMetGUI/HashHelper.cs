namespace KinometGui
{
    public static class HashHelper
    {
        //public static Int32 GetDeterministicHashCode(this string str)
        //{
        //    unchecked
        //    {
        //        Int32 hash1 = (6980 << 16) + 5382;
        //        Int32 hash2 = hash1;

        //        for (int i = 0; i < str.Length; i += 2)
        //        {
        //            hash1 = ((hash1 << 5) + hash1) ^ str[i];
        //            if (i == str.Length - 1)
        //                break;
        //            hash2 = ((hash2 << 5) + hash2) ^ str[i + 1];
        //        }

        //        return hash1 + (hash2 * 1566083943);
        //    }
        //}
        //public static Int32 ToHash(this string str)
        //{
        //    HashCode c = new HashCode();
        //    for (int i = 0; i < str.Length; i++)
        //    {
        //        char x = str[i];
        //        c.Add(x + "1");
        //    }

        //    return c.ToHashCode();
        //}
    }
}

