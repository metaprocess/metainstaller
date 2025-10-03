import { getWsUrl } from '@/utils/env'

export interface WebSocketMessage {
  level: LogEntry["level"] /* 'info' */;
  message: string /* '{"message":"Sudo access not available","success":false}' */;
  operation: string /* 'handleTestSudo' */;
  timestamp: number /* 1754995900283 */;
  type: "log" /* 'log' */;
}

export interface LogEntry {
  containerId?: string
  message: string
  timestamp: string
  level?: 'info' | 'warn' | 'error' | 'debug'
  operation?: string // Added operation field
}

export class WebSocketService {
  private ws: WebSocket | null = null
  private reconnectAttempts = 0
  private maxReconnectAttempts = 1000
  private reconnectDelay = 1000
  private listeners: Map<string, Set<(data: any) => void>> = new Map()

  connect(path: string = '/ws/logs'/* , callback_msg: (msg: string) => void = (_: string) => { } */): Promise<void> {
    return new Promise((resolve, reject) => {
      try {
        const ss = getWsUrl(path);
        this.ws = new WebSocket(getWsUrl(path))

        this.ws.onopen = () => {
          console.log('WebSocket connected')
          this.reconnectAttempts = 0
          resolve()
        }

        this.ws.onmessage = (event) => {
          try {
            /* const EXAMPLE_OF_onmessage = { 
              "level": "info", 
              "message": "{\\"message\\":\\"Sudo access not available\\",\
              \"success\\":false}", 
              "operation": "handleTestSudo", 
              "timestamp": 1754995768194, 
              "type": "log" 
            } */
            const message: WebSocketMessage = JSON.parse(event.data)
            this.handleMessage(message)
          } catch (error) {
            console.error('Error parsing WebSocket message:', error)
          }
        }

        this.ws.onclose = (event) => {
          console.log('WebSocket disconnected:', event.code, event.reason)
          this.handleReconnect()
        }

        this.ws.onerror = (error) => {
          console.error('WebSocket error:', error)
          reject(error)
        }
      } catch (error) {
        reject(error)
      }
    })
  }

  private handleMessage(message: WebSocketMessage) {
    const listeners = this.listeners.get(message.type)
    if (listeners) {
      listeners.forEach(callback => {
        return callback(message)
      })
    }
  }

  private handleReconnect() {
    if (this.reconnectAttempts < this.maxReconnectAttempts) {
      setTimeout(() => {
        this.reconnectAttempts++
        console.log(`Attempting to reconnect... (${this.reconnectAttempts}/${this.maxReconnectAttempts})`)
        this.connect()
      }, this.reconnectDelay * (this.reconnectAttempts + 1))
    } else {
      console.error('Max reconnection attempts reached')
    }
  }

  subscribe(messageType: string, callback: (data: any) => void) {
    if (!this.listeners.has(messageType)) {
      this.listeners.set(messageType, new Set())
    }
    this.listeners.get(messageType)!.add(callback)

    return () => {
      const listeners = this.listeners.get(messageType)
      if (listeners) {
        listeners.delete(callback)
        if (listeners.size === 0) {
          this.listeners.delete(messageType)
        }
      }
    }
  }

  send(message: WebSocketMessage) {
    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
      this.ws.send(JSON.stringify(message))
    } else {
      console.warn('WebSocket is not connected')
    }
  }

  disconnect() {
    if (this.ws) {
      this.ws.close()
      this.ws = null
    }
    this.listeners.clear()
  }

  isConnected(): boolean {
    return this.ws?.readyState === WebSocket.OPEN
  }
}

export const wsService = new WebSocketService()