import { useState, useEffect, useRef } from 'react'
import { wsService, LogEntry, WebSocketMessage } from '@/services/websocket'
import { Button } from '@/components/ui'
import { Terminal, ChevronUp, ChevronDown, X, Trash2, Wifi, WifiOff } from 'lucide-react'
import { cn } from '@/utils/cn'

interface BottomPaneProps {
  className?: string
}

const BottomPane = ({ className }: BottomPaneProps) => {
  const [isExpanded, setIsExpanded] = useState(false)
  const [isConnected, setIsConnected] = useState(false)
  const [logs, setLogs] = useState<LogEntry[]>([])
  const [connectionStatus, setConnectionStatus] = useState<'disconnected' | 'connecting' | 'connected' | 'error'>('disconnected')
  const [lastAnimatedIndex, setLastAnimatedIndex] = useState<number>(-1);
  const prevLogsLengthRef = useRef<number>(logs.length);
  const logsEndRef = useRef<HTMLDivElement>(null)
  const oneShot = useRef<boolean>(false);

  useEffect(() => {
    const connectWebSocket = async () => {
      if (connectionStatus === 'connected' || connectionStatus === 'connecting') return

      setConnectionStatus('connecting')
      try {
        await wsService.connect()
        setIsConnected(true)
        setConnectionStatus('connected')

        // Add connection success log
        setLogs(prev => [...prev, {
          message: 'WebSocket connected successfully',
          timestamp: new Date().toISOString(),
          level: 'info'
        }])
      } catch (error) {
        console.error('WebSocket connection failed:', error)
        setIsConnected(false)
        setConnectionStatus('error')

        // Add connection error log
        setLogs(prev => [...prev, {
          message: `WebSocket connection failed: ${error}`,
          timestamp: new Date().toISOString(),
          level: 'error'
        }])
      }
    }

    if (!oneShot.current) {
      connectWebSocket();
      oneShot.current = true;
    }

    /* return () => {
      wsService.disconnect()
      setIsConnected(false)
      setConnectionStatus('disconnected')
    } */
  }, [/* connectionStatus */])

  useEffect(() => {
    if (!isConnected) return

    const unsubscribe = wsService.subscribe('system', (data: WebSocketMessage) => {
      const logEntry: LogEntry = {
        message: typeof data === 'string' ? data : JSON.stringify(data),
        timestamp: new Date().toISOString(),
        level: 'info',
        operation: data.operation // Add operation if present
      }

      setLogs(prevLogs => {
        const newLogs = [...prevLogs, logEntry]
        // Keep only the last 100 entries
        if (newLogs.length > 100) {
          return newLogs.slice(-100)
        }
        return newLogs
      })
      // Update the last animated index to trigger animation for new logs
      setLastAnimatedIndex(prev => prev + 1)
    })

    const display_logs = wsService.subscribe('log', (data: WebSocketMessage) => {
      const msg = data.message;
      const logEntry: LogEntry = {
        message: typeof msg === 'string' ? msg : JSON.stringify(msg),
        timestamp: new Date(data.timestamp).toISOString(),
        level: data.level,
        operation: data.operation // Add operation if present
      }

      setLogs(prevLogs => {
        const newLogs = [...prevLogs, logEntry]
        // Keep only the last 100 entries
        if (newLogs.length > 100) {
          return newLogs.slice(-100)
        }
        return newLogs
      })
      // Update the last animated index to trigger animation for new logs
      setLastAnimatedIndex(prev => prev + 1)
    })

    const unsubscribeInstall = wsService.subscribe('installation', (data: WebSocketMessage) => {
      const logEntry: LogEntry = {
        message: `Installation: ${data.message || JSON.stringify(data)}`,
        timestamp: new Date().toISOString(),
        level: data.level ? 'error' : 'info',
        operation: data.operation // Add operation if present
      }

      setLogs(prevLogs => {
        const newLogs = [...prevLogs, logEntry]
        if (newLogs.length > 100) {
          return newLogs.slice(-100)
        }
        return newLogs
      })
      // Update the last animated index to trigger animation for new logs
      setLastAnimatedIndex(prev => prev + 1)
    })

    return () => {
      unsubscribe()
      unsubscribeInstall()
      display_logs()
    }
  }, [isConnected])

  useEffect(() => {
    // Update the previous logs length ref
    prevLogsLengthRef.current = logs.length;
    
    // Update lastAnimatedIndex when new logs are added
    if (logs.length > prevLogsLengthRef.current) {
      setLastAnimatedIndex(logs.length - 1);
    }
    
    if (isExpanded && logsEndRef.current) {
      logsEndRef.current.scrollIntoView({ behavior: 'smooth' });
    }
  }, [logs, isExpanded]);

  const handleReconnect = () => {
    setConnectionStatus('disconnected')
    wsService.disconnect()
  }

  const handleClearLogs = () => {
    setLogs([])
  }

  const getStatusColor = () => {
    switch (connectionStatus) {
      case 'connected': return 'text-green-500'
      case 'connecting': return 'text-yellow-500'
      case 'error': return 'text-red-500'
      default: return 'text-gray-500'
    }
  }

  const getStatusIcon = () => {
    switch (connectionStatus) {
      case 'connected': return <Wifi className="h-4 w-4" />
      case 'connecting': return <Wifi className="h-4 w-4 animate-pulse" />
      case 'error': return <WifiOff className="h-4 w-4" />
      default: return <WifiOff className="h-4 w-4" />
    }
  }

  const getLogLevelColor = (level?: string) => {
    switch (level?.toLowerCase()) {
      case 'error': return 'text-red-400'
      case 'warn': return 'text-yellow-400'
      case 'debug': return 'text-gray-400'
      case 'info':
      default: return 'text-green-400'
    }
  }

  return (
    <div className={cn('fixed bottom-0 left-0 right-0 bg-gray-900 text-white border-t border-gray-700 z-50', className)}>
      <style>{`
        @keyframes newLogFlash {
          0% { background-color: rgba(59, 130, 246, 0.3); }
          50% { background-color: rgba(59, 130, 246, 0.1); }
          100% { background-color: transparent; }
        }
        .animate-new-log {
          animation: newLogFlash 1.5s ease-out;
        }
      `}</style>
      {/* Header Bar */}
      <div className="flex items-center justify-between px-4 py-2 bg-gray-800 cursor-pointer" onClick={() => setIsExpanded(!isExpanded)}>
        <div className="flex items-center space-x-3">
          <Terminal className="h-4 w-4" />
          <span className="text-sm font-medium">WebSocket Logs</span>
          <div className={cn('flex items-center space-x-1 text-xs', getStatusColor())}>
            {getStatusIcon()}
            <span>{connectionStatus}</span>
          </div>
          <div className="text-xs text-gray-400">
            {logs.length} entries
          </div>
        </div>

        <div className="flex items-center space-x-2">
          {connectionStatus === 'error' && (
            <Button
              size="sm"
              variant="outline"
              onClick={(e) => {
                e.stopPropagation()
                handleReconnect()
              }}
              className="text-xs h-6"
            >
              Reconnect
            </Button>
          )}

          <Button
            size="sm"
            variant="ghost"
            onClick={(e) => {
              e.stopPropagation()
              handleClearLogs()
            }}
            className="text-xs h-6 px-2"
          >
            <Trash2 className="h-3 w-3" />
          </Button>

          <Button
            size="sm"
            variant="ghost"
            className="h-6 px-2"
          >
            {isExpanded ? <ChevronDown className="h-4 w-4" /> : <ChevronUp className="h-4 w-4" />}
          </Button>
        </div>
      </div>

      {/* Expanded Content */}
      {isExpanded && (
        <div className="flex flex-col h-64 bg-gray-900">
          {logs.length === 0 ? (
            <div className="flex items-center justify-center h-full text-gray-500">
              <div className="text-center">
                <Terminal className="h-8 w-8 mx-auto mb-2 opacity-50" />
                <p className="text-sm">No log entries yet</p>
                <p className="text-xs">WebSocket events will appear here</p>
              </div>
            </div>
          ) : (
            <div className="flex flex-col h-full">
              {/* Table Header - Sticky */}
              <div className="bg-gray-800">
                <table className="min-w-full text-xs font-mono rounded">
                  <thead>
                    <tr>
                      <th className="px-2 py-1 text-left text-gray-400 w-1/6">Time</th>
                      <th className="px-2 py-1 text-left text-gray-400 w-1/6">Level</th>
                      <th className="px-2 py-1 text-left text-gray-400 w-1/6">Operation</th>
                      <th className="px-2 py-1 text-left text-gray-400 w-3/6">Message</th>
                    </tr>
                  </thead>
                </table>
              </div>
              
              {/* Table Body - Scrollable */}
              <div className="overflow-auto flex-grow">
                <table className="min-w-full text-xs font-mono">
                  <tbody>
                    {logs.map((log, index) => (
                      <tr 
                        key={index} 
                        className={cn(
                          "hover:bg-gray-800",
                          index > lastAnimatedIndex ? "animate-new-log" : ""
                        )}
                      >
                        <td className="px-2 py-1 text-gray-400 whitespace-nowrap w-1/6">{new Date(log.timestamp).toLocaleTimeString()}</td>
                        <td className={cn('px-2 py-1 font-semibold w-1/6', getLogLevelColor(log.level))}>[{log.level?.toUpperCase()}]</td>
                        <td className="px-2 py-1 text-blue-400 w-1/6">{log.operation || '-'}</td>
                        <td className={cn('px-2 py-1 break-all w-3/6', getLogLevelColor(log.level))}>{log.message}</td>
                      </tr>
                    ))}
                    <tr><td colSpan={4}><div ref={logsEndRef} /></td></tr>
                  </tbody>
                </table>
              </div>
            </div>
          )}
        </div>
      )}
    </div>
  )
}

export default BottomPane
