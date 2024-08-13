using Editor.Utilities;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Data;
using System.Windows.Threading;

namespace Editor.Content
{
    enum ImportStatus
    {
        Importing,
        Succeeded,
        Failed
    }

    class ImportingItem : ViewModelBase
    {
        public DispatcherTimer _timer;
        private Stopwatch _stopwatch;

        private string _ImportDuration;
        public string ImportDuration
        {
            get => _ImportDuration;
            private set
            {
                if (_ImportDuration != value)
                {
                    _ImportDuration = value;
                    OnPropertyChanged(nameof(ImportDuration));
                }
            }
        }

        public string Name { get; }
        public Asset Asset { get; }

        private ImportStatus _Status;
        public ImportStatus Status
        {
            get => _Status;
            set
            {
                if (_Status != value)
                {
                    _Status = value;
                    OnPropertyChanged(nameof(Status));
                }
            }
        }

        private double _ProgressMaximum;
        public double ProgressMaximum
        {
            get => _ProgressMaximum;
            private set
            {
                if (!_ProgressMaximum.IsEquals(value))
                {
                    _ProgressMaximum = value;
                    OnPropertyChanged(nameof(ProgressMaximum));
                }
            }
        }

        private double _ProgressValue;
        public double ProgressValue
        {
            get => _ProgressValue;
            private set
            {
                if (!_ProgressValue.Equals(value))
                {
                    _ProgressValue = value;
                    OnPropertyChanged(nameof(ProgressValue));
                }
            }
        }

        private double _NormalizedValues;
        public double NormalizedValues
        {
            get => _NormalizedValues;
            private set
            {
                if (!_NormalizedValues.IsEquals(value))
                {
                    _NormalizedValues = value;
                    OnPropertyChanged(nameof(NormalizedValues));
                }
            }
        }

        public void SetProgress(int progress, int maxValue)
        {
            ProgressMaximum = maxValue;
            ProgressValue = progress;
            NormalizedValues = maxValue > 0 ? Math.Clamp(progress / maxValue, 0, 1) : 0;
        }

        private void UpdateTimer(object sender, EventArgs e)
        {
            if (Status == ImportStatus.Importing)
            {
                if (!_stopwatch.IsRunning) _stopwatch.Start();

                var t = _stopwatch.Elapsed;
                ImportDuration = string.Format("{0:00}:{1:00}:{2:00}", t.Minutes, t.Seconds, t.Milliseconds / 10);
            }
            else
            {
                _timer.Stop();
                _stopwatch.Stop();
            }
        }

        public ImportingItem(string name, Asset asset)
        {
            Debug.Assert(!string.IsNullOrEmpty(name) && asset != null);
            Asset = asset;
            Name = name;

            Application.Current.Dispatcher.Invoke(() =>
            {
                _stopwatch = new();
                _timer = new();
                _timer.Interval = TimeSpan.FromMilliseconds(100);
                _timer.Tick += UpdateTimer;
                _timer.Start();
            });
        }
    }

    static class ImportingItemsCollection
    {
        private static ObservableCollection<ImportingItem> _ImportingItems;
        public static ReadOnlyObservableCollection<ImportingItem> ImportingItems { get; private set; }

        public static CollectionViewSource FilteredItems { get; private set; }

        private static readonly object _lockObj = new();
        private static AssetType _ItemFilter = AssetType.Mesh;

        public static void SetItemFilter(AssetType assetType)
        {
            _ItemFilter = assetType;
            FilteredItems.View.Refresh();
        }

        public static void Add(ImportingItem item)
        {
            lock (_lockObj) { Application.Current.Dispatcher.Invoke(() => _ImportingItems.Add(item)); }
        }

        public static void Remove(ImportingItem item)
        {
            lock (_lockObj) { Application.Current.Dispatcher.Invoke(() => _ImportingItems.Remove(item)); }
        }

        public static void Clear(AssetType assetType)
        {
            lock (_lockObj)
            {
                Application.Current.Dispatcher.Invoke(() =>
                {
                    foreach (var item in _ImportingItems.Where(x=>x.Asset.Type == assetType).ToList())
                    {
                        _ImportingItems.Remove(item);
                    }
                });
            }
        }

        public static ImportingItem GetItem(Asset asset)
        {
            lock (_lockObj) { return _ImportingItems.FirstOrDefault(x => x.Asset == asset); }
        }

        /// <summary>
        /// Calling this on a UI thread makes sure all collections are created on the same thread;
        /// </summary>
        public static void Init() { }

        static ImportingItemsCollection()
        {
            _ImportingItems = new();
            ImportingItems = new(_ImportingItems);
            FilteredItems = new() { Source = ImportingItems };
            FilteredItems.Filter += (s, e) =>
            {
                var type = (e.Item as ImportingItem).Asset.Type;
                e.Accepted = type == _ItemFilter;
            };
        }
    }
}
