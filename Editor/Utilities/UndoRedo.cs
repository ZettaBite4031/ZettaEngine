﻿using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Editor.Utilities
{
    public interface IUndoRedo 
    {
        string Name { get; }
        void Undo();
        void Redo();
    }

    public class UndoRedoAction : IUndoRedo
    {
        private Action _undoAction;
        private Action _redoAction;

        public string Name { get; }

        public void Undo() => _undoAction();

        public void Redo() => _redoAction();

        public UndoRedoAction(string name)
        {
            Name = name;
        }

        public UndoRedoAction(Action undo, Action redo, string name) : this(name)
        {
            Debug.Assert(undo != null && redo != null);
            _undoAction = undo;
            _redoAction = redo;
        }

        public UndoRedoAction(string property, object instance, object undo, object redo, string name) :
            this(
                () => instance.GetType().GetProperty(property).SetValue(instance, undo),
                () => instance.GetType().GetProperty(property).SetValue(instance, redo),
                name)
        {

        }
    }

    public class UndoRedo
    {
        private bool _enableAdd = true;
        private readonly ObservableCollection<IUndoRedo> _undoList = new ObservableCollection<IUndoRedo>();
        private readonly ObservableCollection<IUndoRedo> _redoList = new ObservableCollection<IUndoRedo>();
        public ReadOnlyObservableCollection<IUndoRedo> UndoList { get; }
        public ReadOnlyObservableCollection<IUndoRedo> RedoList { get; }

        public void Reset()
        {
            _redoList.Clear();
            _undoList.Clear();
        }

        public void Add(IUndoRedo cmd)
        {
            if (!_enableAdd) return;
            _undoList.Add(cmd);
            _redoList.Clear();
        }

        public void Undo()
        {
            if (_undoList.Any())
            {
                var cmd = _undoList.Last();
                _undoList.RemoveAt(_undoList.Count - 1);
                _enableAdd = false;
                cmd.Undo();
                _enableAdd = true;
                _redoList.Insert(0, cmd);
            }
        }

        public void Redo()
        {
            if (_redoList.Any())
            {
                var cmd = _redoList.First();
                _redoList.RemoveAt(0);
                _enableAdd = false;
                cmd.Redo();
                _enableAdd = true;
                _undoList.Add(cmd);
            }
        }

        public UndoRedo()
        {
            UndoList = new ReadOnlyObservableCollection<IUndoRedo>(_undoList);
            RedoList = new ReadOnlyObservableCollection<IUndoRedo>(_redoList);
        }
    }
}
