import { useState, useEffect, useRef } from 'react'
import { 
  Folder, 
  File, 
  ChevronRight, 
  Home, 
  HardDrive,
  X,
  Check,
  RefreshCw,
  ArrowLeft
} from 'lucide-react'
import { cn } from '@/utils/cn'
import { Button } from './Button'

interface FileInfo {
  name: string
  is_directory: boolean
  size: number
  last_modified: string
}

interface ApiResponse {
  success: boolean
  path: string
  entries: FileInfo[]
  message?: string
}

interface MetaFilePickerProps {
  className?: string
  onPathSelect?: (path: string) => void
  onCancel?: () => void
  initialPath?: string
  title?: string
  allowFileSelection?: boolean
  allowDirectorySelection?: boolean
  placeholder?: string
}

interface BrowsingDirectoryResponse {
  success: boolean
  directory_path: string
}

const MetaFilePicker = ({
  className,
  onPathSelect,
  onCancel,
  initialPath = '/',
  title = 'Select Path',
  allowFileSelection = true,
  allowDirectorySelection = true,
  placeholder = 'Browse and select a path...'
}: MetaFilePickerProps) => {
  const [currentPath, setCurrentPath] = useState<string>(initialPath)
  const [entries, setEntries] = useState<FileInfo[]>([])
  const [loading, setLoading] = useState<boolean>(false)
  const [error, setError] = useState<string>('')
  const [selectedPath, setSelectedPath] = useState<string>('')
  const [selectedIsDirectory, setSelectedIsDirectory] = useState<boolean>(false)
  const [pathHistory, setPathHistory] = useState<string[]>([initialPath])
  const [historyIndex, setHistoryIndex] = useState<number>(0)
  const [filterQuery, setFilterQuery] = useState<string>('')
  const filterInputRef = useRef<HTMLInputElement>(null)

  const fetchDirectoryContents = async (path: string) => {
    setLoading(true)
    setError('')

    try {
      const response = await fetch('/api/file/list-detailed', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({ path })
      })

      const data: ApiResponse = await response.json()

      if (data.success) {
        setEntries(data.entries || [])
        setCurrentPath(data.path)
      } else {
        setError(data.message || 'Failed to load directory contents')
        setEntries([])
      }
    } catch (err) {
      setError('Network error occurred')
      setEntries([])
    } finally {
      setLoading(false)
    }
  }

  const fetchLastBrowsingDirectory = async (): Promise<string | null> => {
    try {
      const response = await fetch('/api/settings/browsing-directory', {
        method: 'GET',
        headers: {
          'Content-Type': 'application/json',
        }
      })

      const data: BrowsingDirectoryResponse = await response.json()

      if (data.success && data.directory_path) {
        return data.directory_path
      }
    } catch (err) {
      console.error('Failed to fetch last browsing directory:', err)
    }
    return null
  }

  const saveBrowsingDirectory = async (directory: string) => {
    try {
      await fetch('/api/settings/browsing-directory', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({ directory_path: directory })
      })
    } catch (err) {
      console.error('Failed to save browsing directory:', err)
    }
  }

  const navigateToPath = (newPath: string) => {
    if (newPath !== currentPath) {
      const newHistory = pathHistory.slice(0, historyIndex + 1)
      newHistory.push(newPath)
      setPathHistory(newHistory)
      setHistoryIndex(newHistory.length - 1)
      setFilterQuery('')
    }
    fetchDirectoryContents(newPath)
  }

  const navigateBack = () => {
    if (historyIndex > 0) {
      setHistoryIndex(historyIndex - 1)
      setFilterQuery('')
      fetchDirectoryContents(pathHistory[historyIndex - 1])
    }
  }

  const navigateForward = () => {
    if (historyIndex < pathHistory.length - 1) {
      setHistoryIndex(historyIndex + 1)
      setFilterQuery('')
      fetchDirectoryContents(pathHistory[historyIndex + 1])
    }
  }

  const getPathSegments = (path: string): string[] => {
    if (path === '/') return ['/']
    return path.split('/').filter(Boolean)
  }

  const buildPathFromSegments = (segments: string[], upToIndex: number): string => {
    if (upToIndex === 0 && segments[0] === '/') return '/'
    const pathParts = segments.slice(0, upToIndex + 1)
    return pathParts[0] === '/' ? '/' + pathParts.slice(1).join('/') : '/' + pathParts.join('/')
  }

  const formatFileSize = (bytes: number): string => {
    if (bytes === 0) return ''
    const units = ['B', 'KB', 'MB', 'GB']
    const i = Math.floor(Math.log(bytes) / Math.log(1024))
    return `${(bytes / Math.pow(1024, i)).toFixed(1)} ${units[i]}`
  }

  const handleItemClick = (item: FileInfo) => {
    const fullPath = currentPath === '/' ? `/${item.name}` : `${currentPath}/${item.name}`
    
    if (item.is_directory) {
      navigateToPath(fullPath)
    } else if (allowFileSelection) {
      setSelectedPath(fullPath)
    }
  }

  const handleItemSelect = (item: FileInfo) => {
    const fullPath = currentPath === '/' ? `/${item.name}` : `${currentPath}/${item.name}`

    if ((item.is_directory && allowDirectorySelection) || (!item.is_directory && allowFileSelection)) {
      setSelectedPath(fullPath)
      setSelectedIsDirectory(item.is_directory)
    }
  }

  const handleConfirmSelection = async () => {
    if (selectedPath && onPathSelect) {
      onPathSelect(selectedPath)
      // Save the directory (parent if file selected, or the directory itself)
      const directoryToSave = selectedIsDirectory ? selectedPath : selectedPath.substring(0, selectedPath.lastIndexOf('/') || 1)
      await saveBrowsingDirectory(directoryToSave)
    }
  }

  const handleSelectCurrentDirectory = () => {
    if (allowDirectorySelection) {
      setSelectedPath(currentPath)
      setSelectedIsDirectory(true)
    }
  }

  useEffect(() => {
    const initializeDirectory = async () => {
      const lastDir = await fetchLastBrowsingDirectory()
      const startPath = lastDir || initialPath
      fetchDirectoryContents(startPath)
    }
    initializeDirectory()
  }, [initialPath])

  useEffect(() => {
    if (entries.length >= 0) {
      filterInputRef.current?.focus()
    }
  }, [entries])

  const filteredEntries = entries.filter((item) =>
    item.name.toLowerCase().includes(filterQuery.toLowerCase())
  )

  return (
    <div className={cn(
      'w-full max-w-4xl mx-auto bg-white rounded-lg border border-gray-200 shadow-lg overflow-hidden',
      className
    )}>
      {/* Header */}
      <div className="bg-gray-50 border-b border-gray-200 p-4">
        <div className="flex items-center justify-between mb-3">
          <h3 className="text-lg font-semibold text-gray-900">{title}</h3>
          {onCancel && (
            <Button
              variant="ghost"
              size="sm"
              onClick={onCancel}
              className="h-8 w-8 p-0"
            >
              <X className="h-4 w-4" />
            </Button>
          )}
        </div>

        {/* Navigation Controls */}
        <div className="flex items-center gap-2 mb-3">
          <Button
            variant="outline"
            size="sm"
            onClick={navigateBack}
            disabled={historyIndex <= 0}
            className="h-8 px-2"
          >
            <ArrowLeft className="h-4 w-4" />
          </Button>
          
          <Button
            variant="outline"
            size="sm"
            onClick={() => navigateToPath('/')}
            className="h-8 px-2"
          >
            <Home className="h-4 w-4" />
          </Button>
          
          <Button
            variant="outline"
            size="sm"
            onClick={() => fetchDirectoryContents(currentPath)}
            disabled={loading}
            className="h-8 px-2"
          >
            <RefreshCw className={cn("h-4 w-4", loading && "animate-spin")} />
          </Button>
        </div>

        {/* Breadcrumbs */}
        <div className="flex items-center gap-1 text-sm text-gray-600 bg-white rounded border px-3 py-2">
          <HardDrive className="h-4 w-4 mr-1" />
          {getPathSegments(currentPath).map((segment, index, array) => (
            <div key={index} className="flex items-center">
              <button
                onClick={() => navigateToPath(buildPathFromSegments(array, index))}
                className="hover:text-blue-600 hover:underline transition-colors"
              >
                {segment === '/' ? 'Root' : segment}
              </button>
              {index < array.length - 1 && (
                <ChevronRight className="h-4 w-4 mx-1 text-gray-400" />
              )}
            </div>
          ))}
        </div>

        {/* Filter Input */}
        <div className="mt-3">
          <input
            ref={filterInputRef}
            type="text"
            placeholder="Filter files and directories..."
            value={filterQuery}
            onChange={(e) => setFilterQuery(e.target.value)}
            className="w-full px-3 py-2 text-sm border border-gray-300 rounded focus:outline-none focus:ring-2 focus:ring-blue-500 focus:border-transparent"
            autoFocus
          />
        </div>

        {/* Selected Path Display */}
        {selectedPath && (
          <div className="mt-3 p-2 bg-blue-50 border border-blue-200 rounded text-sm">
            <span className="text-blue-700 font-medium">Selected: </span>
            <span className="text-blue-900">{selectedPath}</span>
          </div>
        )}
      </div>

      {/* Content Area */}
      <div className="h-96 overflow-y-auto">
        {loading && (
          <div className="flex items-center justify-center h-full">
            <RefreshCw className="h-6 w-6 animate-spin text-gray-400" />
            <span className="ml-2 text-gray-600">Loading...</span>
          </div>
        )}

        {error && (
          <div className="flex items-center justify-center h-full">
            <div className="text-center">
              <div className="text-red-600 mb-2">Error: {error}</div>
              <Button
                variant="outline"
                size="sm"
                onClick={() => fetchDirectoryContents(currentPath)}
              >
                Retry
              </Button>
            </div>
          </div>
        )}

        {!loading && !error && (
          <div className="divide-y divide-gray-100">
            {/* Current Directory Selection Option */}
            {allowDirectorySelection && (
              <div
                onClick={handleSelectCurrentDirectory}
                className={cn(
                  "flex items-center p-3 hover:bg-gray-50 cursor-pointer transition-colors group",
                  selectedPath === currentPath && "bg-blue-50"
                )}
              >
                <div className="flex items-center mr-3">
                  <input
                    type="radio"
                    checked={selectedPath === currentPath}
                    onChange={() => handleSelectCurrentDirectory()}
                    className="text-blue-600"
                  />
                </div>
                <Folder className="h-5 w-5 text-blue-600 mr-3 flex-shrink-0" />
                <div className="flex-1 min-w-0">
                  <div className="text-sm font-medium text-gray-900">
                    . (Select this directory)
                  </div>
                  <div className="text-xs text-gray-500">
                    {currentPath}
                  </div>
                </div>
              </div>
            )}

            {/* Directory and File Entries */}
            {filteredEntries.map((item) => {
              const fullPath = currentPath === '/' ? `/${item.name}` : `${currentPath}/${item.name}`
              const canSelect = (item.is_directory && allowDirectorySelection) || (!item.is_directory && allowFileSelection)
              
              return (
                <div
                  key={item.name}
                  className={cn(
                    "flex items-center p-3 hover:bg-gray-50 transition-colors group",
                    selectedPath === fullPath && "bg-blue-50"
                  )}
                >
                  {/* Selection Radio/Checkbox */}
                  {canSelect && (
                    <div className="flex items-center mr-3">
                      <input
                        type="radio"
                        checked={selectedPath === fullPath}
                        onChange={() => handleItemSelect(item)}
                        className="text-blue-600"
                      />
                    </div>
                  )}

                  {/* Icon */}
                  <div className="mr-3 flex-shrink-0">
                    {item.is_directory ? (
                      <Folder className="h-5 w-5 text-blue-600" />
                    ) : (
                      <File className="h-5 w-5 text-gray-500" />
                    )}
                  </div>

                  {/* Content */}
                  <div 
                    className="flex-1 min-w-0 cursor-pointer"
                    onClick={() => handleItemClick(item)}
                  >
                    <div className="text-sm font-medium text-gray-900 truncate">
                      {item.name}
                      {item.is_directory && (
                        <ChevronRight className="h-4 w-4 inline ml-1 text-gray-400 opacity-0 group-hover:opacity-100 transition-opacity" />
                      )}
                    </div>
                    <div className="text-xs text-gray-500 flex items-center gap-3">
                      <span>{item.last_modified}</span>
                      {!item.is_directory && item.size > 0 && (
                        <span>{formatFileSize(item.size)}</span>
                      )}
                    </div>
                  </div>
                </div>
              )
            })}

            {filteredEntries.length === 0 && !loading && !error && (
              <div className="flex items-center justify-center h-32 text-gray-500">
                <div className="text-center">
                  <Folder className="h-8 w-8 mx-auto mb-2 opacity-50" />
                  <div>
                    {filterQuery && entries.length > 0 ? 'No matches found' : 'This directory is empty'}
                  </div>
                </div>
              </div>
            )}
          </div>
        )}
      </div>

      {/* Footer */}
      <div className="bg-gray-50 border-t border-gray-200 p-4">
        <div className="flex items-center justify-between">
          <div className="text-sm text-gray-600">
            {selectedPath ? `Selected: ${selectedPath}` : placeholder}
          </div>
          
          <div className="flex gap-2">
            {onCancel && (
              <Button
                variant="outline"
                size="sm"
                onClick={onCancel}
              >
                Cancel
              </Button>
            )}
            
            <Button
              variant="primary"
              size="sm"
              onClick={handleConfirmSelection}
              disabled={!selectedPath}
            >
              <Check className="h-4 w-4 mr-1" />
              Select
            </Button>
          </div>
        </div>
      </div>
    </div>
  )
}

MetaFilePicker.displayName = 'MetaFilePicker'

export { MetaFilePicker }
