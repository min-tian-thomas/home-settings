import datetime as dt
import logging
from abc import abstractmethod
from dataclasses import dataclass
from typing import Optional

import pandas as pd

logger = logging.getLogger(__name__)


@dataclass
class Event:
    time: dt.datetime
    data: pd.DataFrame


class EventAggregator:
    @abstractmethod
    def on_event(self, event):
        pass

    @abstractmethod
    def get_aggregated(self, time: dt.datetime) -> Optional[Event]:
        pass


class LastEventAggregator(EventAggregator):
    def __init__(self):
        self._last_event = None

    def on_event(self, event):
        self._last_event = event

    def get_aggregated(self):
        return self._last_event


class EventSource:
    def __init__(
        self,
        name: str,
        interval_seconds: int,
        start_time: dt.datetime,
        aggregator: EventAggregator = None,
    ):
        self.name = name
        self.interval = dt.timedelta(seconds=interval_seconds)
        self.start_time = start_time
        self.next_trigger_time = start_time + self.interval
        self.aggregator = aggregator if aggregator else LastEventAggregator()

    def on_evnet(self, event: Event) -> Optional[Event]:
        if event.time <= self.start_time:
            logger.info(
                "Event with time=%s before %s will be dropped",
                event.time,
                self.start_time,
            )
            return
        self.aggregator.on_event(event)
        if event.time == self.next_trigger_time:
            aggregated = self.aggregator.get_aggregated()
            self.next_trigger_time += self.interval
            return aggregated
        return None


class EventsSyncer:
    def __init__(self, interval_seconds: int, start_time: dt.datetime):
        self.interval_seconds = interval_seconds
        self.start_time = start_time
        self.event_sources: dict[str, EventSource] = {}

    def add_event_source(self, event_source: EventSource) -> None:
        self.event_sources
