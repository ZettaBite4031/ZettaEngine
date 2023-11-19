﻿using Editor.Content;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Security.Cryptography;
using System.Text;
using System.Threading.Tasks;
using System.Threading.Tasks.Dataflow;
using System.Windows;
using System.Windows.Media;
using System.Xaml;

namespace Editor
{
    static class VisualExtensions
    {
        public static T FindVisualParent<T>(this DependencyObject dpo) where T : DependencyObject
        {
            if (!(dpo is Visual)) return null;
           
            var parent = VisualTreeHelper.GetParent(dpo);
            while (parent != null)
            {
                if (parent is T type) return type;
                parent = VisualTreeHelper.GetParent(parent);
            }
            return null;
        }
    }

    public static class ContentHelper
    {
        public static string GetRandomString(int length = 8)
        {
            if (length <= 0) length = 8;
            var n = length / 11;
            var sb = new StringBuilder();
            for (int i = 0; i <= n; i++) sb.Append(Path.GetRandomFileName().Replace(".","")); 
            return sb.ToString(0, length);
        }

        public static bool IsDirectory(string path)
        {
            try
            {
                return File.GetAttributes(path).HasFlag(FileAttributes.Directory);
            }
            catch (Exception ex) { Debug.WriteLine(ex.Message); }
            return false;
        }

        public static bool IsDirectory(this FileInfo info) => info.Attributes.HasFlag(FileAttributes.Directory);

        public static bool IsOlder(this DateTime date, DateTime other) => date < other;

        public static string SanitizeFileName(string v)
        {
            var path = new StringBuilder(v.Substring(0, v.LastIndexOf(Path.DirectorySeparatorChar) + 1));
            var file = new StringBuilder(v[(v.LastIndexOf(Path.DirectorySeparatorChar) + 1)..]);
            foreach (var c in Path.GetInvalidPathChars()) path.Replace(c, '_');
            foreach (var c in Path.GetInvalidFileNameChars()) file.Replace(c, '_');
            return path.Append(file).ToString();
        }

        internal static byte[] ComputeHash(byte[] data, int offset = 0, int count = 0)
        {
            if (data?.Length > 0)
            {
                using var sha256 = SHA256.Create();
                return sha256.ComputeHash(data, offset, count > 0 ? count : data.Length);
            }
            return null;
        }

        internal static async Task ImportFilesAsync(string[] files, string destination)
        {
            try
            {
                Debug.Assert(!string.IsNullOrEmpty(destination));
                ContentWatcher.EnableFileWatcher(false);
                var tasks = files.Select(async file => await Task.Run(() => { Import(file, destination); }));
                await Task.WhenAll(tasks);
            }
            catch (Exception ex)
            {
                Debug.WriteLine($"Failed to import files to {destination}");
                Debug.WriteLine(ex.Message);
            }
            finally
            {
                ContentWatcher.EnableFileWatcher(true);
            }
        }

        private static void Import(string file, string destination)
        {
            Debug.Assert(!string.IsNullOrEmpty(file));
            if (IsDirectory(file)) return;
            if (!destination.EndsWith(Path.DirectorySeparatorChar)) destination += Path.DirectorySeparatorChar; 
            var name = Path.GetFileNameWithoutExtension(file);
            var ext = Path.GetExtension(file).ToLower();

            Asset asset = null;

            switch (ext)
            {
                case ".fbx":
                    asset = new Content.Geometry();
                    break;
                case ".bmp": break;
                case ".png": break;
                case ".jpg": break;
                case ".jpeg": break;
                case ".jiff": break;
                case ".tif": break;
                case ".tga": break;
                case ".wav": break;
                case ".ogg": break;
                default:
                    break;
            }

            if (asset != null) Import(asset, name, file, destination);
        }

        private static void Import(Asset asset, string name, string file, string destination)
        {
            Debug.Assert(asset != null);
            asset.FullPath = destination + name + Asset.AssetFileExtension;
            if (!string.IsNullOrEmpty(file)) asset.Import(file);

            asset.Save(asset.FullPath);
            return;
        }
    }
}
