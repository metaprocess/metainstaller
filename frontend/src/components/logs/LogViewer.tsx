import { useState, useEffect, useRef } from 'react'
import { useContainers } from '@/hooks/useApi'
import { wsService, LogEntry } from '@/services/websocket'
import { Card, CardContent, CardDescription, CardHeader, CardTitle, Button, Input } from '@/components/ui'
import { Badge } from '@/components/ui'
import { Terminal, Play, Square, Search, Download, Trash2 } from 'lucide-react'
import type { Container } from '@/types/api'

const LogViewer = () => {
  const [selectedContainer, setSelectedContainer] = useState<string>('')
  const [logs, setLogs] = useState<LogEntry[]>([])
  const [isConnected, setIsConnected] = useState(false)
  const [isStreaming, setIsStreaming] = useState(false)
  const [searchTerm, setSearchTerm] = useState('')
  const [maxLines, setMaxLines] = useState(1000)
  const logsEndRef = useRef<HTMLDivElement>(null)
  const logsContainerRef = useRef<HTMLDivElement>(null)

  const { data: containers } = useContainers(true)

  useEffect(() => {
    /* const connectWebSocket = async () => {
      try {
        await wsService.connect()
        setIsConnected(true)
      } catch (error) {
        console.error('WebSocket connection failed:', error)
        setIsConnected(false)
      }
    }

    connectWebSocket()

    return () => {
      wsService.disconnect()
    } */
  }, [])

  useEffect(() => {
    if (!isConnected) return

    const unsubscribe = wsService.subscribe('logs', (data: LogEntry) => {
      setLogs(prevLogs => {
        const newLogs = [...prevLogs, data]
        // Keep only the last maxLines entries
        if (newLogs.length > maxLines) {
          return newLogs.slice(-maxLines)
        }
        return newLogs
      })
    })

    return unsubscribe
  }, [isConnected, maxLines])

  useEffect(() => {
    if (isStreaming && logsEndRef.current) {
      logsEndRef.current.scrollIntoView({ behavior: 'smooth' })
    }
  }, [logs, isStreaming])

  const handleStartStreaming = () => {
    if (!selectedContainer) return
    
    setIsStreaming(true)
    setLogs([])
    
    // Send start streaming message
    wsService.send({
      type: 'start_logs',
      data: { containerId: selectedContainer }
    })
  }

  const handleStopStreaming = () => {
    setIsStreaming(false)
    
    // Send stop streaming message
    wsService.send({
      type: 'stop_logs',
      data: { containerId: selectedContainer }
    })
  }

  const handleClearLogs = () => {
    setLogs([])
  }

  const handleDownloadLogs = () => {
    const logText = logs.map(log => 
      `[${log.timestamp}] ${log.level?.toUpperCase() || 'INFO'}: ${log.message}`
    ).join('\n')
    
    const blob = new Blob([logText], { type: 'text/plain' })
    const url = URL.createObjectURL(blob)
    const a = document.createElement('a')
    a.href = url
    a.download = `${selectedContainer || 'docker'}-logs-${new Date().toISOString().split('T')[0]}.txt`
    document.body.appendChild(a)
    a.click()
    document.body.removeChild(a)
    URL.revokeObjectURL(url)
  }

  const filteredLogs = logs.filter(log =>
    log.message.toLowerCase().includes(searchTerm.toLowerCase())
  )

  const getLogLevelColor = (level?: string) => {
    switch (level?.toLowerCase()) {
      case 'error': return 'text-red-600'
      case 'warn': return 'text-yellow-600'
      case 'debug': return 'text-gray-500'
      case 'info':
      default: return 'text-gray-800'
    }
  }

  const getLogLevelBadge = (level?: string) => {
    const variant = level?.toLowerCase() === 'error' ? 'destructive' :
                   level?.toLowerCase() === 'warn' ? 'warning' :
                   level?.toLowerCase() === 'debug' ? 'secondary' : 'default'
    
    return (
      <Badge variant={variant} className="text-xs">
        {level?.toUpperCase() || 'INFO'}
      </Badge>
    )
  }

  return (
    <div className="space-y-6">
      <div>
        <h1 className="text-3xl font-bold">Log Viewer</h1>
        <p className="text-gray-600 mt-2">Real-time container logs with WebSocket streaming</p>
      </div>

      {/* Connection Status */}
      <Card>
        <CardContent className="p-4">
          <div className="flex items-center justify-between">
            <div className="flex items-center space-x-2">
              <div className={`w-3 h-3 rounded-full ${isConnected ? 'bg-green-500' : 'bg-red-500'}`}></div>
              <span className="text-sm">
                WebSocket: {isConnected ? 'Connected' : 'Disconnected'}
              </span>
            </div>
            <div className="flex items-center space-x-2">
              <div className={`w-3 h-3 rounded-full ${isStreaming ? 'bg-blue-500 animate-pulse' : 'bg-gray-400'}`}></div>
              <span className="text-sm">
                Streaming: {isStreaming ? 'Active' : 'Inactive'}
              </span>
            </div>
          </div>
        </CardContent>
      </Card>

      {/* Controls */}
      <Card>
        <CardHeader>
          <CardTitle>Log Controls</CardTitle>
          <CardDescription>Configure log streaming and filtering</CardDescription>
        </CardHeader>
        <CardContent className="space-y-4">
          <div className="grid gap-4 md:grid-cols-2 lg:grid-cols-4">
            <div>
              <label className="text-sm font-medium mb-2 block">Container</label>
              <select
                className="w-full p-2 border rounded-lg text-sm"
                value={selectedContainer}
                onChange={(e) => setSelectedContainer(e.target.value)}
              >
                <option value="">Select container...</option>
                {containers?.containers.map((container: Container) => (
                  <option key={container.id} value={container.id}>
                    {container.name} ({container.id.substring(0, 12)})
                  </option>
                ))}
              </select>
            </div>

            <div>
              <label className="text-sm font-medium mb-2 block">Max Lines</label>
              <Input
                type="number"
                value={maxLines}
                onChange={(e) => setMaxLines(parseInt(e.target.value) || 1000)}
                min="100"
                max="10000"
                step="100"
              />
            </div>

            <div>
              <label className="text-sm font-medium mb-2 block">Actions</label>
              <div className="flex space-x-1">
                {!isStreaming ? (
                  <Button
                    size="sm"
                    onClick={handleStartStreaming}
                    disabled={!selectedContainer || !isConnected}
                  >
                    <Play className="h-3 w-3 mr-1" />
                    Start
                  </Button>
                ) : (
                  <Button
                    size="sm"
                    variant="destructive"
                    onClick={handleStopStreaming}
                  >
                    <Square className="h-3 w-3 mr-1" />
                    Stop
                  </Button>
                )}
                <Button
                  size="sm"
                  variant="outline"
                  onClick={handleClearLogs}
                >
                  <Trash2 className="h-3 w-3 mr-1" />
                  Clear
                </Button>
              </div>
            </div>

            <div>
              <label className="text-sm font-medium mb-2 block">Export</label>
              <Button
                size="sm"
                variant="outline"
                onClick={handleDownloadLogs}
                disabled={logs.length === 0}
              >
                <Download className="h-3 w-3 mr-1" />
                Download
              </Button>
            </div>
          </div>

          <div className="relative">
            <Search className="absolute left-3 top-3 h-4 w-4 text-gray-400" />
            <Input
              placeholder="Search logs..."
              value={searchTerm}
              onChange={(e) => setSearchTerm(e.target.value)}
              className="pl-9"
            />
          </div>
        </CardContent>
      </Card>

      {/* Log Display */}
      <Card className="flex flex-col h-[600px]">
        <CardHeader className="flex-shrink-0">
          <div className="flex items-center justify-between">
            <div>
              <CardTitle className="flex items-center space-x-2">
                <Terminal className="h-5 w-5" />
                <span>Container Logs</span>
              </CardTitle>
              <CardDescription>
                {selectedContainer ? 
                  `Showing logs for ${containers?.containers.find(c => c.id === selectedContainer)?.name || selectedContainer.substring(0, 12)}` :
                  'Select a container to view logs'
                }
              </CardDescription>
            </div>
            <div className="flex items-center space-x-2 text-sm text-gray-500">
              <span>Total: {logs.length}</span>
              <span>•</span>
              <span>Filtered: {filteredLogs.length}</span>
            </div>
          </div>
        </CardHeader>
        <CardContent className="flex-1 p-0">
          <div 
            ref={logsContainerRef}
            className="h-full overflow-auto bg-gray-900 text-green-400 p-4 font-mono text-sm"
          >
            {filteredLogs.length === 0 ? (
              <div className="flex items-center justify-center h-full text-gray-500">
                {!selectedContainer ? (
                  <div className="text-center">
                    <Terminal className="h-12 w-12 mx-auto mb-4 opacity-50" />
                    <p>Select a container to start viewing logs</p>
                  </div>
                ) : !isStreaming ? (
                  <div className="text-center">
                    <p>Click "Start" to begin log streaming</p>
                  </div>
                ) : (
                  <div className="text-center">
                    <p>Waiting for log entries...</p>
                  </div>
                )}
              </div>
            ) : (
              <div className="space-y-1">
                {filteredLogs.map((log, index) => (
                  <div key={index} className="flex items-start space-x-2 hover:bg-gray-800 px-2 py-1 rounded">
                    <span className="text-gray-400 text-xs whitespace-nowrap">
                      {new Date(log.timestamp).toLocaleTimeString()}
                    </span>
                    <div className="flex-shrink-0">
                      {getLogLevelBadge(log.level)}
                    </div>
                    <span className={`break-all ${getLogLevelColor(log.level)}`}>
                      {log.message}
                    </span>
                  </div>
                ))}
                <div ref={logsEndRef} />
              </div>
            )}
          </div>
        </CardContent>
      </Card>
    </div>
  )
}

export default LogViewer